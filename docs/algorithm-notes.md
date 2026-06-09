# Algorithm Notes

This project implements a teaching-oriented solver for first- and second-order constant-coefficient linear nonhomogeneous ODEs.

The current solver focuses on the undetermined-coefficients method in a matrix form:

1. Write the equation as `L(D)y = f(x)`.
2. Build the characteristic polynomial `F(lambda)`.
3. Identify the nonhomogeneous term type.
4. Detect the multiplicity `j` of `r` or `a + bi` as a characteristic root.
5. Use
   `L(D)(x^q e^(rx)) = e^(rx) sum C(q,i) F^(i)(r) x^(q-i)`
   to construct the coefficient system.
6. Solve the coefficient system over rational or complex-rational numbers.
7. Merge the homogeneous solution and particular solution.

## Supported Right-Hand Side Forms

- `P(x)e^(rx)`
- `e^(ax)(R(x)cos bx + I(x)sin bx)`

## Design Direction

The code intentionally avoids calling a general CAS in the current stage. The goal is to make the paper's method visible and testable as a dedicated teaching algorithm.

The expression parser is currently a mapping layer from a restricted equation syntax to `SolverInput`. It is not a full symbolic algebra system yet.

Planned work:

- Square-root exact form for characteristic roots.
- Expression parser shared by keyboard input and button input.
- LaTeX export/rendering for results and derivation steps.
- GeoGebra-style input panel.
