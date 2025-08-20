# Screenshots

This directory contains screenshots demonstrating the raymarching demo running on both platforms.


- `vita.png` - Screenshot from PS Vita showing the raymarched scene
- `linux.png` - Screenshot from Linux showing the identical raymarched scene

## Expected Visual Content

Both screenshots should show:

- A gray ground plane
- A red reflective sphere (with visible reflections)
- A blue non-reflective sphere (static)
- Sky gradient background
- Real-time PBR lighting and materials

**Note:** The shaders look a bit different between platforms due to PS Vita's hardware limitations. The Vita version has reduced raymarching iterations, simplified effects, and some optimizations to run smoothly on the mobile GPU. But the core rendering and visual result should be very similar!
