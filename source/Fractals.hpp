#pragma once

#include "OpenGLHelper.hpp"

namespace fractals
{
	struct Viewport
	{
		Viewport(const glm::dvec4& viewport = glm::dvec4(0.0)) :
			viewport(viewport)
		{

		}

		Viewport(const double left, const double right, const double bottom, const double top) :
			left(left), right(right), bottom(bottom), top(top)
		{

		}

		union
		{
			glm::dvec4 viewport;

			struct
			{
				double left, right, bottom, top;
			};
		};
	};

	class Fractal
	{
	public:
		virtual void iterate(const std::int32_t iterations = 1) = 0;

		virtual void render(const glm::ivec2 resolution, const Viewport& viewport) = 0;

		virtual void reset() = 0;
	};

	struct AffineTransform
	{
		glm::mat2x2 matrix;
		glm::vec2 offset;
	};

	class Affine : public Fractal
	{
	private:
		std::vector<std::uint32_t> pixels;
		RAIIWrapper<GLuint> textureSet;

		RAIIWrapper<GLuint> framebufferIterate;
		RAIIWrapper<GLuint> programIterate;
		GLuint counter;
		GLint locationCounter;

		RAIIWrapper<GLuint> textureColor;
		RAIIWrapper<GLuint> programRender;
		glm::ivec2 size;
		GLint locationSize;
		GLint locationResolution;
		Viewport viewport;
		GLint locationViewport;
		GLint locationViewportRequested;

		static inline std::int32_t findBestIndex(const std::int32_t lower, const std::int32_t upper, const std::int32_t size, const float begin, const float end, float value)
		{
			if (lower >= upper - 1)
			{
				float fLower = begin + (float(lower) + 0.5f) / float(size) * (end - begin);
				float fUpper = begin + (float(upper) + 0.5f) / float(size) * (end - begin);

				float distance1 = glm::abs(fLower - value);
				float distance2 = glm::abs(fUpper - value);

				if (distance1 <= distance2)
				{
					return lower;
				}
				else
				{
					return upper;
				}
			}

			std::int32_t middle = (upper + lower) / 2;

			float fMiddle = begin + (float(middle) + 0.5f) / float(size) * (end - begin);

			if (value <= fMiddle)
			{
				return findBestIndex(lower, middle, size, begin, end, value);
			}
			else
			{
				return findBestIndex(middle, upper, size, begin, end, value);
			}
		}

		static inline glm::ivec2 findBestPixel(const glm::ivec2& lower, const glm::ivec2& upper, const glm::ivec2& size, const Viewport& viewport, const glm::vec2& point)
		{
			std::int32_t ix = findBestIndex(lower.x, upper.x, size.x, viewport.left, viewport.right, point.x);
			std::int32_t iy = findBestIndex(lower.y, upper.y, size.y, viewport.bottom, viewport.top, point.y);

			return glm::ivec2(ix, iy);
		}

	public:
		Affine(const glm::ivec2& size, const Viewport& viewport = {}, const std::vector<AffineTransform>& affineTransforms = {}, const std::vector<glm::vec2>& initialPoints = { glm::vec2(0.0f) }, std::function<bool(float, float)> indicator = nullptr) :
			counter(0), size(size), viewport(viewport)
		{
			this->textureSet = RAIIWrapper<GLuint>(glCreate(Texture)(), glDelete(Texture));

			glBindTexture(GL_TEXTURE_2D, this->textureSet);

			this->pixels = std::vector<std::uint32_t>(size.x * size.y, std::numeric_limits<std::uint32_t>::max());

			if (indicator)
			{
				for (std::int32_t iy = 0; iy < size.y; iy++)
				{
					for (std::int32_t ix = 0; ix < size.x; ix++)
					{
						float x = viewport.left + (float(ix) + 0.5f) / float(size.x) * (viewport.right - viewport.left);
						float y = viewport.bottom + (float(iy) + 0.5f) / float(size.y) * (viewport.top - viewport.bottom);

						if (indicator(x, y))
						{
							pixels[ix + size.x * iy] = this->counter;
						}
					}
				}
			}

			for (const auto& point : initialPoints)
			{
				glm::ivec2 pixel = this->findBestPixel(glm::ivec2(0), size - 1, size, viewport, point);

				pixels[pixel.x + size.x * pixel.y] = this->counter;
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, size.x, size.y, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, this->pixels.data());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


			this->framebufferIterate = RAIIWrapper<GLuint>(glCreate(Framebuffer)(), glDelete(Framebuffer));

			glBindFramebuffer(GL_FRAMEBUFFER, this->framebufferIterate);

			this->textureColor = RAIIWrapper<GLuint>(glCreate(Texture)(), glDelete(Texture));

			glBindTexture(GL_TEXTURE_2D, this->textureColor);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->textureColor, 0);

			GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };

			glDrawBuffers(1, drawBuffers);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				throw std::runtime_error("GL-Error: Framebuffer not completed.");
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			auto vertexShaderCode = CODE(
				#version 420 core \n

				vec2 vertices[4] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));
				int indices[6] = int[](0, 1, 2, 1, 2, 3);

				void main()
				{
					gl_Position = vec4(vertices[indices[gl_VertexID]] * 2.0 - vec2(1.0), 0.0, 1.0);
				}
			);

			gl::requireExtension("GL_ARB_shader_image_load_store");

			auto fragmentShaderCode = CODE(
				#version 420 core \n
				#extension GL_ARB_shader_image_load_store : enable \n

				precision highp float;

				layout(binding = 0) uniform usampler2D sampler;
				layout(r32ui, binding = 0) uniform uimage2D img;

				uniform uint counter;

				out vec4 color;
			);

			fragmentShaderCode += "ivec2 size = ivec2(" + std::to_string(size.x) + ", " + std::to_string(size.y) + ");";
			fragmentShaderCode += "vec4 viewport = vec4(" + std::to_string(viewport.left) + ", " + std::to_string(viewport.right) + ", " + std::to_string(viewport.bottom) + ", " + std::to_string(viewport.top) + ");";

			fragmentShaderCode += CODE(
				vec2 fromScreen(const vec2 screen)
				{
					return viewport.xz + screen * (viewport.yw - viewport.xz);
				}

				vec2 toScreen(const vec2 pos)
				{
					return (pos - viewport.xz) / (viewport.yw - viewport.xz);
				}

				void main()
				{
					vec2 screen = gl_FragCoord.xy / size;

					uint value = texture(sampler, screen).r;

					if (value == counter - 1u)
					{
						vec2 pos = fromScreen(screen);
						vec2 newPos;

						mat2x2 matrix;
						vec2 offset;
			);

			for (const auto& affineTransform : affineTransforms)
			{
				fragmentShaderCode += "matrix[0] = vec2(" + std::to_string(affineTransform.matrix[0].x) + ", " + std::to_string(affineTransform.matrix[0].y) + ");";
				fragmentShaderCode += "matrix[1] = vec2(" + std::to_string(affineTransform.matrix[1].x) + ", " + std::to_string(affineTransform.matrix[1].y) + ");";
				fragmentShaderCode += "offset = vec2(" + std::to_string(affineTransform.offset.x) + ", " + std::to_string(affineTransform.offset.y) + ");";

				fragmentShaderCode += CODE(
						newPos = matrix * pos + offset;

						imageAtomicMin(img, ivec2(toScreen(newPos) * size), counter);
				);
			}

			fragmentShaderCode += CODE(
					}

					memoryBarrier();

					color = vec4(0, 5 * exp(-float(texture(sampler, screen).r) / 10.0), 0, 1.0);
				}
			);

			this->programIterate = gl::compileAndLinkShaders(vertexShaderCode, fragmentShaderCode);

			this->locationCounter = glGetUniformLocation(this->programIterate, "counter");

			
			fragmentShaderCode = CODE(
				#version 420 core \n

				precision highp float;

				uniform sampler2D sampler;

				uniform ivec2 size;
				uniform ivec2 resolution;

				uniform vec4 viewport;
				uniform vec4 viewportRequested;

				in vec2 texCoords;

				out vec4 color;

				vec2 fromScreen(const vec2 screen, const vec4 viewport)
				{
					return viewport.xz + screen * (viewport.yw - viewport.xz);
				}

				vec2 toScreen(const vec2 pos, const vec4 viewport)
				{
					return (pos - viewport.xz) / (viewport.yw - viewport.xz);
				}

				void main()
				{
					vec2 screen = gl_FragCoord.xy / resolution;

					vec2 pos = toScreen(fromScreen(screen, viewportRequested), viewport);

					color = vec4(texture(sampler, pos));
				}
			);

			this->programRender = gl::compileAndLinkShaders(vertexShaderCode, fragmentShaderCode);

			this->locationSize = glGetUniformLocation(this->programRender, "size");
			this->locationResolution = glGetUniformLocation(this->programRender, "resolution");

			this->locationViewport = glGetUniformLocation(this->programRender, "viewport");
			this->locationViewportRequested = glGetUniformLocation(this->programRender, "viewportRequested");
		}

		virtual void reset() override
		{
			glBindTexture(GL_TEXTURE_2D, this->textureSet);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, size.x, size.y, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, this->pixels.data());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, this->framebufferIterate);

			glClear(GL_COLOR_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glBindTexture(GL_TEXTURE_2D, this->textureColor);

			glGenerateMipmap(GL_TEXTURE_2D);

			this->counter = 0;
		}

		virtual void iterate(const std::int32_t iterations) override
		{
			glBindFramebuffer(GL_FRAMEBUFFER, this->framebufferIterate);

			for (std::int32_t i = 0; i < iterations; i++)
			{
				glViewport(0, 0, this->size.x, this->size.y);

				glUseProgram(this->programIterate);

				glUniform1ui(this->locationCounter, ++this->counter);

				glBindTexture(GL_TEXTURE_2D, this->textureSet);

				glBindImageTexture(0, this->textureSet, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);

				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glBindTexture(GL_TEXTURE_2D, this->textureColor);

			glGenerateMipmap(GL_TEXTURE_2D);
		}

		virtual void render(const glm::ivec2 resolution, const Viewport& viewport) override
		{
			glUseProgram(this->programRender);

			glUniform2iv(this->locationSize, 1, reinterpret_cast<const GLint*>(&this->size));
			glUniform2iv(this->locationResolution, 1, reinterpret_cast<const GLint*>(&resolution));

			glm::vec4 fViewport(this->viewport.viewport);
			glm::vec4 fViewportRequested(viewport.viewport);

			glUniform4fv(this->locationViewport, 1, reinterpret_cast<const GLfloat*>(&fViewport));
			glUniform4fv(this->locationViewportRequested, 1, reinterpret_cast<const GLfloat*>(&fViewportRequested));

			glBindTexture(GL_TEXTURE_2D, this->textureColor);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	};

	class BarnsleyFern : public Affine
	{
	private:
		inline static std::vector<AffineTransform> createAffineTransforms()
		{
			return std::vector<fractals::AffineTransform>(
				{
					{ glm::mat2x2(glm::vec2(+0.00f, +0.00f), glm::vec2(+0.00f, +0.16f)), glm::vec2(+0.00f, +0.00f) },
					{ glm::mat2x2(glm::vec2(+0.85f, -0.04f), glm::vec2(+0.04f, +0.85f)), glm::vec2(+0.00f, +1.60f) },
					{ glm::mat2x2(glm::vec2(+0.20f, +0.23f), glm::vec2(-0.26f, +0.22f)), glm::vec2(+0.00f, +1.60f) },
					{ glm::mat2x2(glm::vec2(-0.15f, +0.26f), glm::vec2(+0.28f, +0.24f)), glm::vec2(+0.00f, +0.44f) }
				}
			);
		}

	public:
		BarnsleyFern(const glm::ivec2& size) :
			Affine(size, Viewport(-2.2f, 2.7f, 0.0f, 10.0f), this->createAffineTransforms())
		{

		}
	};

	class SierpinskiTriangle : public Affine
	{
	private:
		inline static std::vector<AffineTransform> createAffineTransforms()
		{
			return std::vector<fractals::AffineTransform>(
				{
					{ glm::mat2x2(0.5f), glm::vec2(0.0f) },
					{ glm::mat2x2(0.5f), glm::vec2(0.5f, 0.0f) },
					{ glm::mat2x2(0.5f), glm::vec2(0.0f, 0.5f) }
				}
			);
		}

	public:
		SierpinskiTriangle(const glm::ivec2& size) :
			Affine(size, Viewport(0.0f, 1.0f, 0.0f, 1.0f), this->createAffineTransforms())
		{

		}
	};
}
