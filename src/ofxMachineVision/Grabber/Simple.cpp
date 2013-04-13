#include "Simple.h"

namespace ofxMachineVision {
	namespace Grabber {
		//----------
		Simple::Simple(Device::Base * device) : Base(device) {
			this->newFrameWaiting = false;
			this->currentFrameNew = false;
			this->useTexture = true;
		}

		//----------
		void Simple::open(int deviceID) {
			this->close();

			try {
				switch (this->getDeviceType()) {
				case Device::Type_Blocking:
					this->threadBlocking->open(deviceID);
					this->deviceState = this->getSpecification().getValid() ? State_Running : State_Closed;
					ofAddListener(this->threadBlocking->evtNewFrame, this, &Simple::callbackNewFrame);
					break;
				default:
					throw std::exception("Device not implemented");
				}
			} catch (std::exception e) {
				OFXMV_ERROR << e.what();
				this->deviceState = State_Closed;
			}
		}

		//----------
		void Simple::close() {
			if (this->getIsDeviceRunning()) {
				switch (this->getDeviceType()) {
				case Device::Type_Blocking:
					this->threadBlocking->close();
					this->deviceState = State_Closed;
					ofRemoveListener(this->threadBlocking->evtNewFrame, this, &Simple::callbackNewFrame);
					cout << __func__ << " closing"; 
					break;
				}
			}
		}

		//----------
		void Simple::startCapture(const TriggerMode & triggerMode, const TriggerSignalType & triggerSignalType) {
			CHECK_OPEN

			this->setTriggerMode(triggerMode, triggerSignalType);
		
			switch (this->getDeviceType()) {
			case Device::Type_Blocking:
				this->threadBlocking->startFreeRun();
				this->deviceState = State_Running;
				break;
			}
		}

		//----------
		void Simple::stopCapture() {
			CHECK_OPEN
			switch (this->getDeviceType()) {
				case Device::Type_Blocking:
					this->threadBlocking->stopFreeRun();
					break;
			}
		}

		//----------
		void Simple::update() {
			currentFrameNew = this->newFrameWaiting;
			this->newFrameWaiting = false;

			if (this->isFrameNew()) {
				this->waitingPixelsLock.lock();
				std::swap(this->pixels, this->waitingPixels);
				this->waitingPixelsLock.unlock();

				if (this->useTexture) {
					if ((int) this->texture.getWidth() != this->pixels.getWidth() || (int) this->texture.getHeight() != this->pixels.getHeight()) {
						this->texture.allocate(this->pixels);
					}

					this->texture.loadData(this->pixels);
				}
			}
		}

		//----------
		void Simple::draw(float x, float y, float w, float h) {
			this->texture.draw(x, y, w, h);
		}

		//----------
		float Simple::getWidth() {
			return this->pixels.getWidth();
		}

		//----------
		float Simple::getHeight() {
			return this->pixels.getHeight();
		}

		//----------
		void Simple::setUseTexture(bool useTexture) {
			this->useTexture = useTexture;

			if (!useTexture) {
				this->texture.clear();
			}
		}

		//----------
		unsigned char * Simple::getPixels() {
			return this->getPixelsRef().getPixels();
		}

		//----------
		ofPixels & Simple::getPixelsRef() {
			return this->pixels;
		}

		//----------
		void Simple::setBinning(int binningX, int binningY) {
			CHECK_OPEN
			REQUIRES(Feature_Binning)

			switch (this->getDeviceType()) {
			case Device::Type_Blocking:
				this->threadBlocking->setBinning(binningX, binningY);
				break;
			}
		}

		//----------
		void Simple::setROI(const ofRectangle & roi) {
			CHECK_OPEN
			REQUIRES(Feature_ROI);

			switch (this->getDeviceType()) {
			case Device::Type_Blocking:
				this->threadBlocking->setROI(roi);
				break;
			}
		}

		//----------
		void Simple::setTriggerMode(const TriggerMode & triggerMode, const TriggerSignalType & triggerSignalType) {
			CHECK_OPEN
			REQUIRES(Feature_Triggering)
			REQUIRES(triggerMode);
			REQUIRES(triggerSignalType);

			switch (this->getDeviceType()) {
			case Device::Type_Blocking:
				this->threadBlocking->setTriggerMode(triggerMode, triggerSignalType);
			}
		}

		clock_t lastFrame = clock();

		//----------
		void Simple::callbackNewFrame(Frame & frame) {
			if (this->getDeviceState() == State_Deleting) {
				return;
			}

			frame.lockForReading();
			this->waitingPixelsLock.lock();
			this->waitingPixels = frame.getPixelsRef();
			this->waitingPixelsLock.unlock();
			frame.unlock();

			float interval = clock() - lastFrame;
			interval /= float(CLOCKS_PER_SEC);
			cout << "Frame captured in " << interval << "s\t" << 1.0f / interval << "fps." << endl;

			lastFrame = clock();

			this->newFrameWaiting = true;
		}
	}
}