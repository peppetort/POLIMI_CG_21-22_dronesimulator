#version 450

layout(set = 1,binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragViewDir;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in float v_fogDepth;

layout(location = 0) out vec4 outColor;

vec3 Oren_Nayar_Diffuse_BRDF(vec3 L, vec3 N, vec3 V, vec3 C, float sigma) {
	// Directional light direction
	// additional parameter:
	// float sigma : roughness of the material
	float thetai = acos(dot(L,N));
	float thetar = acos(dot(V,N));
	float alpha = max(thetai,thetar);
	float beta = min(thetai,thetar);
	float A = 1-(0.5*pow(sigma,2)/(pow(sigma,2)+0.33));
	float B = 0.45*pow(sigma,2)/(pow(sigma,2)+0.09);
	vec3 vi = normalize(L-(dot(L,N))*N);
	vec3 vr = normalize(V-(dot(V,N))*N);
	float G = max(0,dot(vi,vr));
	vec3 L_ON =  C*clamp(dot(L,N),0.0,1.0);
	return L_ON*(A+B*G*sin(alpha)*tan(beta));
}

void main() {
	const vec3  diffColor = texture(texSampler, fragTexCoord).rgb;
	const vec3  specColor = vec3(1.0f, 1.0f, 1.0f);
	const float specPower = 150.0f;
	const vec3  L = vec3(-0.4830f, 0.8365f, -0.2588f);
	const float roughness = 1.5f;
	
	vec3 N = normalize(fragNorm);
	vec3 R = -reflect(L, N);
	vec3 V = normalize(fragViewDir);

	vec3 diffuse = Oren_Nayar_Diffuse_BRDF(L, N, V, diffColor,roughness) * specColor;
	vec3 ambient  = (vec3(0.1f,0.1f, 0.1f) * (1.0f + N.y) + vec3(0.0f,0.0f, 0.1f) * (1.0f - N.y)) * diffColor;
	
	outColor = vec4(clamp(ambient + diffuse, vec3(0.0f), vec3(1.0f)), 1.0f);
	  
	float fogAmount = smoothstep(0.0f, 70.0f, v_fogDepth);

	outColor = mix(outColor, vec4(0.8, 0.9, 1, 1), fogAmount);
}