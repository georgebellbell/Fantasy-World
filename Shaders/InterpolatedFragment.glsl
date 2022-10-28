#version 330 core

in Vertex{
	vec4 colour;
} IN;

out vec4 fragColour;
void main(void) {

    if ((IN.colour.r > 0.1 && IN.colour.r < 0.9) && 
        (IN.colour.g > 0.1 && IN.colour.g < 0.9) &&
        (IN.colour.b > 0.1 && IN.colour.b < 0.9)){
            discard;
    }

    //if (IN.colour.r <= 0.5 && IN.colour.g <= 0.5 && IN.colour.b <= 0.5) discard;

	fragColour = IN.colour;
}