#include <iostream>
#include <vector>
#include <algorithm>
#include "ppm.hpp"
#include "material.hpp"
#include "hittable.hpp"
#include "camera.hpp"

namespace RT
{
    Camera::Camera(unsigned int imageWidth, unsigned int imageHeight, const Point3& position)
        : m_ImageWidth(imageWidth), m_ImageHeight(imageHeight), m_Position(position), m_SamplesPerPixel(100)
    {
        m_MaxDepth = 50;

        const float focalLength = 1.0f;
        const float viewportHeight = 2.0f;
        const float viewportWidth = viewportHeight * (static_cast<float>(imageWidth) / imageHeight);

        const Vec3 viewportU{viewportWidth, 0.0f, 0.0f};
        const Vec3 viewportV{0.0f, -viewportHeight, 0.0f};

        m_PixelDeltaU = viewportU / static_cast<float>(imageWidth);
        m_PixelDeltaV = viewportV / static_cast<float>(imageHeight);

        const Vec3 viewportUpperLeft =
            m_Position - Vec3(0.0f, 0.0f, focalLength) - (viewportU * 0.5f) - (viewportV * 0.5f);

        m_Pixel00Location = viewportUpperLeft + (m_PixelDeltaU + m_PixelDeltaV) * 0.5f;
    }

    bool Camera::Render(const char* filename, const Hittable& world)
    {
        std::vector<Color> pixels(m_ImageWidth * m_ImageHeight);

        for (unsigned int j = 0; j < m_ImageHeight; ++j) {
            std::cout << "\rScanlines remaining: " << m_ImageHeight - j << " " << std::flush;

            for (unsigned int i = 0; i < m_ImageWidth; ++i) {
                Color pixelColor{0.0f};

                for (unsigned int sample = 0; sample < m_SamplesPerPixel; ++sample) {
                    const Ray ray = GetRay(i, j);

                    pixelColor += TraceRay(ray, m_MaxDepth, world);
                }

                pixelColor /= static_cast<float>(m_SamplesPerPixel);

                // Gamma correct
                pixelColor.x = LinearToGamma(pixelColor.x);
                pixelColor.y = LinearToGamma(pixelColor.y);
                pixelColor.z = LinearToGamma(pixelColor.z);

                pixelColor.x = std::clamp(pixelColor.x, 0.0f, 1.0f);
                pixelColor.y = std::clamp(pixelColor.y, 0.0f, 1.0f);
                pixelColor.z = std::clamp(pixelColor.z, 0.0f, 1.0f);

                pixels[j * m_ImageWidth + i] = pixelColor;
            }
        }

        std::cout << "\rWriting PPM file...          " << std::flush;

        if (!WritePPM(filename, m_ImageWidth, m_ImageHeight, reinterpret_cast<const float*>(pixels.data()))) {
            std::cerr << "Failed to write PPM file: " << filename << std::endl;
            return false;
        }

        std::cout << "\rDone!                      \n" << std::flush;

        return true;
    }

    float Camera::LinearToGamma(float linear)
    {
        return std::pow(linear, 1.0f / 2.2f);
    }

    Color Camera::TraceRay(const Ray& ray, int depth, const Hittable& world)
    {
        if (depth <= 0) {
            return Color{0.0f};
        }

        HitInfo hitInfo;

        if (world.Hit(ray, Interval{0.001f, FltInfinity}, &hitInfo)) {
            Ray scattered;
            Color attenuation;

            if (hitInfo.material->Scatter(ray, hitInfo, &attenuation, &scattered)) {
                return Hadamard(attenuation, TraceRay(scattered, depth - 1, world));
            }

            return Color{0.0f};
        }

        const Vec3 unitRayDirection = Normalize(ray.direction());
        const float a = 0.5f * (unitRayDirection.y + 1.0f);
        return Lerp(Vec3{1.0f}, Vec3{0.5f, 0.7f, 1.0f}, a);
    }

    Ray Camera::GetRay(unsigned int i, unsigned int j)
    {
        const Vec3 pixelCenter = m_Pixel00Location +
            (static_cast<float>(i) * m_PixelDeltaU) + (static_cast<float>(j) * m_PixelDeltaV);
        
        const Vec3 pixelSample = pixelCenter + PixelSampleSquare();

        const Vec3 rayOrigin = m_Position;
        const Vec3 rayDirection = pixelSample - rayOrigin;

        return Ray{rayOrigin, rayDirection};
    }

    const Vec3 Camera::PixelSampleSquare()
    {
        const float px = -0.5f + RandomFloat();
        const float py = -0.5f + RandomFloat();
        return (px * m_PixelDeltaU) + (py * m_PixelDeltaV);
    }
}
