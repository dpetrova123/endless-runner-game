#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;

    vec3 glow;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform float currentTime;

float computeLightIntensity(float time) {
    return 0.5f * (1.0f + sin(time * (2.0f * 3.14159265358979323846 / 2.0f)));
}

void main()
{
    // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

    float lightIntensity = computeLightIntensity(currentTime);
    vec4 texColor = texture(material.specular, TexCoords);
    vec4 texColorS = texture(material.diffuse, TexCoords);
    vec3 invertedTexColor = vec3(1.0) - step(0.01, texColor.rgb);

    vec3 glowingColor = material.glow * (invertedTexColor * texColorS.rgb) * lightIntensity; // Add emissive component

    vec3 result = ambient + diffuse + specular + glowingColor;
    FragColor = vec4(result, 1.0);
}

