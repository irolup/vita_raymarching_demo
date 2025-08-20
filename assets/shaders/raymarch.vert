void main(
    float2 aPos,
    float2 aUV,
    float2 out vUV : TEXCOORD0,
    float4 out gl_Position : POSITION
) {
    vUV = aUV;
    gl_Position = float4(aPos, 0.0f, 1.0f);
}