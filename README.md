# Fractal Renderer
A simple OpenGL accelerated renderer for fractals, which are characterized as fixpoints of contractions (generated by an **iterated function system (IFS)** of affine contraction mappings) on the space of non-empty compact subsets of ℝ².

This project was created as part of a seminar presentation on the topic of _iterated function systems_ in order to enhance it visually.
The implementation is based on the ideas presented in _Fractals Everywhere_ by Michael F. Barnsley.

# Eye Candy
![SierpinskiTriangle](figures/SierpinskiTriangle.png?raw=true)
A square-like set was iterated by the contractions for a Sierpinski triangle, where the current iteration was highlighted by the green color and the previous iterations are still visible in shades of red.
\
\
![Mandelbrot](figures/Mandelbrot.png?raw=true)
For obvious reasons, I could not resist to include a freely zoomable depiction of the Mandelbrot set.

# Build and Run
```
# build
git clone --recursive https://github.com/PeterSilie24/FractalRenderer.git
cd FractalRenderer
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

```
# run
./FractalRenderer
```
