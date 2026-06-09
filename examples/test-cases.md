# Test Cases

Use coefficients from highest degree to constant term.

Formula-input examples:

```text
y''+y=x*cos(2x)
y''-2y'+y=x*e^x
y''+y=cos(x)
```

## Polynomial

Equation:

```text
y'' + y = x^2 - 2
```

Input:

```text
order: 2
a1: 0
a0: 1
kind: P(x)e^(rx)
r: 0
P(x): 1, 0, -2
```

Expected particular solution:

```text
x^2 - 4
```

## Exponential, Non-Resonant

Equation:

```text
y'' - y = e^(2x)
```

Input:

```text
order: 2
a1: 0
a0: -1
kind: P(x)e^(rx)
r: 2
P(x): 1
```

Expected particular solution:

```text
1/3e^(2x)
```

## Exponential, Resonant

Equation:

```text
y'' - y = e^x
```

Input:

```text
order: 2
a1: 0
a0: -1
kind: P(x)e^(rx)
r: 1
P(x): 1
```

Expected particular solution:

```text
1/2xe^x
```

## Exponential Polynomial, Double Resonance

Equation:

```text
y'' - 2y' + y = xe^x
```

Input:

```text
order: 2
a1: -2
a0: 1
kind: P(x)e^(rx)
r: 1
P(x): 1, 0
```

Expected particular solution:

```text
1/6x^3e^x
```

## Trigonometric, Non-Resonant

Equation:

```text
y'' + y = x cos(2x)
```

Input:

```text
order: 2
a1: 0
a0: 1
kind: e^(ax)(R(x)cos bx + I(x)sin bx)
a: 0
b: 2
R(x): 1, 0
I(x): 0
```

Expected particular solution:

```text
-1/3xcos(2x) + 4/9sin(2x)
```

## Trigonometric, Resonant

Equation:

```text
y'' + y = cos x
```

Input:

```text
order: 2
a1: 0
a0: 1
kind: e^(ax)(R(x)cos bx + I(x)sin bx)
a: 0
b: 1
R(x): 1
I(x): 0
```

Expected particular solution:

```text
1/2xsin(x)
```
