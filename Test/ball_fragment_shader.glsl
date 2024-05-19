#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoords;

uniform vec4 color;

void main()
{
    // Create a checkerboard pattern
    float scale = 10.0; // Adjust this value to change the size of the pattern
    float pattern = mod(floor(TexCoords.x * scale) + floor(TexCoords.y * scale), 2.0);

    // Color the fragment based on the pattern
    vec3 patternColor = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), pattern); // Red and Blue

    FragColor = vec4(patternColor * ourColor, 1.0);
}
