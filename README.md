# Realtime-Video-Magnification
An OpenCV/Qt based realtime application to magnify motion and color in videos and camerastreams

## How do I use it?
### Connect
- Camera
    - Device Number: Type in the device number of your camera connected to your computer. Indexing starts with 0 which is usually your built-in webcam.
    - (Ubuntu/Linux) PointGrey Device on USB:  Having a DC1394 Camera connected to your computer, OpenCV redirects the camera over the v4l-API to device number 0. If you wish to also connect to your built-in camera, enable this option to set built-in to 0, DC1394 to 1
    - Image Buffer: Select the length of an image buffer before processing those images. If dropping frames if buffer is full is disabled, your capture rate will be same the same as your processing rate.
- Video
    - Choose a video. Compatibility is given if your computer supports the codec. Valid file endings are .avi .mp4 .m4v .mkv .mov .wmv
- Resolution: This does not work for videos on Ubuntu/Linux yet (Windows not tested). For cameras check the supported modes from camera manufacturer and type in the resolution specified for a mode.
- Frames per Second: Some cameras support multiple modes with different resolution/fps/etc. . Setting the framerate will change into a mode with a framerate near the one you typed in. For videos, some mp4-files have a bad header where OpenCV can't read out the framerate, which will normally be set to 30FPS. Anyway here you can set it manually.
![Connect Dialog](pictures/connect_dialog.png)

### Color Magnification
![Panel for Color Magnification Options](pictures/cmag_options.png)

### Motion Magnification
![Panel for Motion Magnification Options](pictures/lmag_options.png)

### Save

## Dependencies
- [Qt](http://qt-project.org/) >= 5.0
- [OpenCV](http://opencv.org/) >= 2.0 (< 3.0 ? -> not tested yet)

## License
This application is licensed under GPLv3, read the [LICENSE](LICENSE).

## Credits
Thanks to Nick D'Ademo, whose [qt-opencv-multithreaded](https://code.google.com/p/qt-opencv-multithreaded/) application 
served as basis and Joseph Pans [QtEVM](https://github.com/wzpan/QtEVM) application whose algorithms were adapted
for this project.

Also take a look at the MITs webpage for [Eulerian Video Magnification](http://people.csail.mit.edu/mrub/vidmag/). 
They also provide demo videos on their page and the team did a fantastic job in researching and developing this field of science.

## How does it work?
The algorithms are using a combination of spatial filters (e.g. image pyramids) and temporal filters to determine
motions of different spatial wavelength in videos. These can be amplified separately before collapsing the image pyramid
and adding the motion image back to the original.

![Idea of the Video Magnification Algorithm](pictures/magnification.png)

### Color Magnification
![Connect Dialog](pictures/connect_dialog.png)

### Motion Magnification
![Color Magnification UML](pictures/colorMag.png)

![Laplace Magnification UML](pictures/colorMag.png)
