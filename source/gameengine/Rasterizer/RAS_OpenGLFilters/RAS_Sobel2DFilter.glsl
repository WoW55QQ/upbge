uniform sampler2D bgl_RenderedTexture;
uniform vec2 bgl_TextureCoordinateOffset[9];
in vec4 bgl_TexCoord;
out vec4 fragColor;

void main(void)
{
	vec4 sample[9];

	for (int i = 0; i < 9; i++)
	{
		sample[i] = texture(bgl_RenderedTexture,
		                      bgl_TexCoord.xy + bgl_TextureCoordinateOffset[i]);
	}

	vec4 horizEdge = sample[2] + (2.0*sample[5]) + sample[8] -
	        (sample[0] + (2.0*sample[3]) + sample[6]);

	vec4 vertEdge = sample[0] + (2.0*sample[1]) + sample[2] -
	        (sample[6] + (2.0*sample[7]) + sample[8]);

	fragColor.rgb = sqrt((horizEdge.rgb * horizEdge.rgb) +
	                        (vertEdge.rgb * vertEdge.rgb));
	fragColor.a = 1.0;
}

