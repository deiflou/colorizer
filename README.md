# colorizer

LazyBrush implementation in C++ and Qt that tries to speed up the computations by partitioning the image space with a grid of quadtrees

## Colorizer app usage

### I/O Section
* **Open Image** button: opens a new image
* **Save Colorization Image** button: save only the colorized areas without the line drawing
* **Save Composed Image** button: save the line drawing superposed on the colorized areas

### Visualization Mode Section
Select what and how is displayed.
* **Original Image**: display the loaded image
* Skeleton: display the simplified version of the loaded image
* **Space Partitioning**: display how the space is patitioned due to the image points and the scribbles
* **Space Partitioning (Scribbles)**: display how the space is patitioned making enphasis on the scribbles
* **Space Partitioning (Labels)**: display how the space is patitioned making enphasis on the labeling proposed by the scribbles
* **Space Partitioning (Neighbors)**: display how the space is patitioned and the conections between the selected cell and its neighbors. To select a cell in this mode just right click it
* **Labeling**: display the colorized regions only (this is what is saved when clicking the *Save Colorization Image* button)
* **Composed Image**: display original image superposed on the colorized regions (this is what is saved when clicking the *Save Composed Image* button)

### Brush Options Section
* **Size**: sets the maximum size of the brush used to draw scribbles. When using a pen the size is scaled by the pressure
* **Color**: set the color used when a new scribble is drawn if you click with the left mouse button. If you click with the right mouse button the color under the mouse is set/unset to be used as a transparent background

### Other Options Section
* **Use Implicit Surrounding Background Scribble**: If you set this option the effect is as if a transparent background scribble was drawn surrounding the image. This can save you putting a few scribbles used as transparent background

### Keys

* **Left mouse button / Pen**: draw a new scrible
* **Middle mouse button**: pan view
* **Mouse wheel**: zoom view
* **Right mouse button**: select cell in "Space Partitioning (Neighbors)" visualization mode
