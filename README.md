Robot vision with YARP & OpenCV Assignment
==========================

[![GitPitch](https://gitpitch.com/assets/badge.svg)](https://gitpitch.com/vvv-school/assignment_closest-blob/master?grs=github&t=moon)

# Prerequisites
By now, you should be familiar with playing around with images processing such as:
- Getting images from streams
- Smoothing Images
- Eroding and Dilating (Morphology transformation)
- Basic thresholding
- Hough Circle Transform
- Template Matching
- Finding contours in your image
- Image Moments
- Point Polygon Test

documentation & examples on image filtering can be taken from [here](http://docs.opencv.org/3.1.0/d4/d86/group__imgproc__filter.html) & [here](http://docs.opencv.org/2.4/doc/tutorials/imgproc/table_of_content_imgproc/table_of_content_imgproc.html) or from the tutorials. 

more info & examples on 

# Assignment
As an assignment, we would like you to complete a module that employs what we have seen so far in order to extract the closest blob from a stream of disparity images. This will be useful for you when you will do the machine learning course.

You should be able to accomplish the following tasks:

1. Get a **stream of images** from the yarpdataplayer: **disparity** and **rgb**.
1. Apply **image processing** techniques to make the disparity image cleaner for processing. (GaussianBlur, erode, dilate)
1. Retrieve the **maximum value** and its **position**.
1. Apply **thresholding** on that point to remove unwanted background
1. Find the **contour** of the closest object with its **moment** and **mass center**
1. Draw it on the disparity image
1. Get its **bounding box** (ROI-Region of Interest)
1. Create a **cropped image** containing the **RGB image** of the ROI
1. Fill in a **YARP bottle** as a **list**
1. Finally, **stream** the resulting **image** out.

The outcome should look something like the animation below:

![closest-blob](/misc/assignment.gif)

Most of the points reported above have been already addressed in the tutorials, so you need to **fill in the missing gaps** highlighted by the comment `// FILL IN THE CODE`.

Once done, you can test your code in two ways:

1. **Manually**: running the _yarpmanager scripts_ provided from within [**app/scripts**](./app/scripts).
1. **Automatically**: [running the script **test.sh**](https://github.com/vvv-school/vvv-school.github.io/blob/master/instructions/how-to-run-smoke-tests.md) in the **smoke-test** directory. Take into account these important points:
    1. The smoke test basically will automatically compare the content of the streamed bounding box historgrams in hsv at various frames with the base solution.
    1. The test will pass if an overall comparison of the **five** histograms is above 90%.

## IMPORTANT Notes

You will need to run the dataset **dataDisparity** in:
```
cd $ROBOT_CODE
$ cd datasets
$ wget http://www.icub.org/download/software/datasetplayer-demo/dataDisparity.zip
$ unzip dataDisparity.zip
```
# [How to complete the assignment](https://github.com/vvv-school/vvv-school.github.io/blob/master/instructions/how-to-complete-assignments.md)
