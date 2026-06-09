# ODE Calculator

A teaching-oriented Qt/C++ calculator for constant-coefficient linear nonhomogeneous ordinary differential equations.

The project implements a matrix-form undetermined coefficients method, with exact rational arithmetic for the coefficient-solving step.

## Current Features

- First-order equation: `y' + a0 y = f(x)`
- Second-order equation: `y'' + a1 y' + a0 y = f(x)`
- Right-hand side forms:
  - `P(x)e^(rx)`
  - `e^(ax)(R(x)cos bx + I(x)sin bx)`
- Rational input such as `2`, `-0.5`, and `3/4`
- Exact rational and complex-rational Gaussian elimination
- Early formula-input mode, for example:
  - `y''+y=x*cos(2x)`
  - `y''-2y'+y=x*e^x`
  - `y''+y=cos(x)`
- WebEngine-backed formula display for parsed equations, solutions, and derivation steps

## Method

The core method follows the matrix-form undetermined coefficients idea:

1. Write the equation as `L(D)y = f(x)`.
2. Build the characteristic polynomial `F(lambda)`.
3. Detect the multiplicity `j` of `r` or `a + bi` as a characteristic root.
4. Use
   `L(D)(x^q e^(rx)) = e^(rx) sum C(q,i) F^(i)(r) x^(q-i)`
   to construct the coefficient system.
5. Solve the coefficient system over rational or complex-rational numbers.
6. Merge the homogeneous solution and the particular solution.

## Build

Open `CMakeLists.txt` with Qt Creator and choose a Qt 6 MSVC kit.

The project has been tested locally with:

- Qt 6.9.3 MSVC 2022 64-bit
- Qt WebEngine
- Qt WebChannel
- Qt Positioning
- Visual Studio 2022 C++ toolchain
- CMake
- C++17

On Windows, Qt WebEngine is not available for MinGW builds. After building a
Release executable, deploy the runtime files with `windeployqt` from a VS x64
developer environment, for example:

```powershell
cmd /s /c "call ""D:\Visual Studio\VC\Auxiliary\Build\vcvars64.bat"" >nul && ""D:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe"" --release --compiler-runtime ""D:\Qt Projects\ODE_Calculator\build_msvc\Release\ODE_Calculator.exe"""
```

## Roadmap

- Exact square-root support for characteristic roots
- A broader expression parser shared by keyboard input and button input
- Optional KaTeX/MathJax renderer packaging for higher-fidelity LaTeX output
- GeoGebra-style input buttons
