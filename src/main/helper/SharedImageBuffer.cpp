#include "SharedImageBuffer.h"

SharedImageBuffer::SharedImageBuffer() {
  // Initialize variables(s)
  nArrived = 0;
}

void SharedImageBuffer::add(int deviceNumber, Buffer<Mat> *imageBuffer) {
  // Add image buffer to map
  imageBufferMap[deviceNumber] = imageBuffer;
}

Buffer<Mat> *SharedImageBuffer::getByDeviceNumber(int deviceNumber) {
  return imageBufferMap[deviceNumber];
}

void SharedImageBuffer::removeByDeviceNumber(int deviceNumber) {
  // Remove buffer for device from imageBufferMap
  imageBufferMap.remove(deviceNumber);
}

void SharedImageBuffer::wakeAll() {
  QMutexLocker locker(&mutex);
  wc.wakeAll();
}

bool SharedImageBuffer::containsImageBufferForDeviceNumber(int deviceNumber) {
  return imageBufferMap.contains(deviceNumber);
}
