# OpenGL With C 3D Open World


This was done as a solo final project for CSCI 4229.


The project has not been altered since the final class submission, meaning all assignments requirements, comments, errors, etc remain in tact.


To create the executable simply run "make" using MingGW.


- Use arrow keys to adjust the viewing angle
- Press '9' to reset all parameters (not including modes)
- Press '=' to shut off the program
- Use '<' and '>' to adjust the world size
- Use '+' and '-' to adjust the field of view
- Use 'm' or 'M to change the mode (0 = FP, 1 = Perspective, 2 = Orthogonal)
- Use wasd and qe to affect the position of the light (certain movement modes only)
- Use 'n' or 'N' to change the lights movement mode
- Use 'l' to toggle lighting
- Use '1' to toggle smooth and flat shading
- use '2' to toggle local viewer
- use '3' to toggle texture mode
- Use 't' and 'T' to adjust ambient light
- Use 'g' and 'G' to adjust diffuse light
- Use 'b' and 'B' to adjust specular light
- Use 'y' and 'Y' to adjust emitted light
- Use 'h' and 'H' to adjust shininess
- Use 'u' and 'U' to adjust ball increment 
- Use 'p' and 'P' to adjust camera height (FP mode)


All code used was my own or modified/reused example code provided by the instructor.


All aspects of previous homeworks have been utilized and expanded upon to make a neighbourhood w/ a day night cycle, NPC's, proper lighting, coherent structure, and multiple detailed objects.


The world can be explored in first person by default, althouth a view of the entire area can be acessed quickly by pressing 'm' once.


Certain elements of the world (small tree placement, size, and rotation & grass/leaf placement and rotation) are randomized based on the time each time the application is launched.

It's incredibly unlikely but small trees may not appear due to the RNG in a single launch. Please close and relaunch the file should this happen.
