// Generating RLE Sequence for RLE compression
// 
// author : Byungil Jeong
//

uniform sampler2D colorMap;
uniform vec2		winSize;
uniform vec2		blockSize;
uniform float		blockPixels;
uniform vec2		blockDim;

void main(void)
{
   const vec2 offset = vec2(1.0 / winSize.x, 1.0/winSize.y);
	const vec4 zeroColor = vec4(0.0);
	
   vec2 texCoord = gl_TexCoord[0].xy;
	vec2 pixelCoord = floor(texCoord*winSize);
	float pixelIdx = pixelCoord.y * winSize.x + pixelCoord.x;
	float blockIdx = floor(pixelIdx/blockPixels);
	vec2 blockPos = vec2(mod(blockIdx,blockDim.x), floor(blockIdx/blockDim.x));
	float innerIdx = mod(pixelIdx,blockPixels);
	vec2 innerPos = vec2(mod(innerIdx,blockSize.x), floor(innerIdx/blockSize.x));
	
	vec2 finalPos = blockPos*blockSize + innerPos; 
	vec2 myCoord = texCoord + (finalPos - pixelCoord)*offset;
	vec4 myColor = texture2D(colorMap, myCoord);

	vec4 uniquePixel = vec4(1.0, 0.0, 0.0, 0.0);
	vec2 neighborCoord = vec2(myCoord.x - offset.x, myCoord.y);
	vec4 neighborColor = texture2D(colorMap, neighborCoord);
	if (myColor.rgb == neighborColor.rgb && innerPos.x > 0.0)
		uniquePixel.r = 0.0;

	gl_FragData[0] = myColor;
	gl_FragData[1] = uniquePixel;
}
