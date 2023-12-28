
# calco 🦶🏽🍇 - Compression Algorithm for Low-end Computers

Calco[^1] (Compression Algorithm for Low-end COmputers) is a `C++20` library designed for mildly compressing text strings efficiently.

## Purpose

The primary focus of Calco is on execution speed and minimizing instruction count rather than achieving the highest compression ratio. Its intended use case revolves around low-end microcontrollers where ROM size is a limited resource.

## Functionality

This library aims to assist in:

 - Reducing transmitted data size.
 - Shrinking the firmware image size.

Calco achieves the latter by enabling the compression of constant strings during compile time, which can then be decompressed during runtime when required.

## Compression Efficiency

While the maximum achievable compression ratio is around `25%`, in practical scenarios, a more realistic measure falls within the range of `15-20%`.

[^1]: */ˈkal.koː/*, first person of the Latin verb *calcāre*: to press with the foot/to crush the grapes (with the feet).
