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

		virtual Viewport getPreferredViewport() const
		{
			return Viewport(-1.0, 1.0, -1.0, 1.0);
		}

		virtual std::int32_t getPreferredIterationsPerFrame() const
		{
			return 1;
		}

		virtual void options()
		{

		}
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

			this->pixels = std::vector<std::uint32_t>(std::size_t(size.x) * std::size_t(size.y), std::numeric_limits<std::uint32_t>::max());

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

			auto vertexShaderCode = CODE(\
				#version 420 core \n\

				vec2 vertices[4] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));
				int indices[6] = int[](0, 1, 2, 1, 2, 3);

				void main()
				{
					gl_Position = vec4(vertices[indices[gl_VertexID]] * 2.0 - vec2(1.0), 0.0, 1.0);
				}
			);

			gl::requireExtension("GL_ARB_shader_image_load_store");

			auto fragmentShaderCode = CODE(\
				#version 420 core \n\
				#extension GL_ARB_shader_image_load_store : enable \n\

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

			
			fragmentShaderCode = CODE(\
				#version 420 core \n\

				precision highp float;

				uniform sampler2D sampler;

				uniform ivec2 size;
				uniform ivec2 resolution;

				uniform vec4 viewport;
				uniform vec4 viewportRequested;

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

		virtual Viewport getPreferredViewport() const override
		{
			return Viewport(-2.2f, 2.7f, 0.0f, 10.0f);
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

		virtual Viewport getPreferredViewport() const override
		{
			return Viewport(0.0f, 1.0f, 0.0f, 1.0f);
		}
	};

	class Mandelbrot : public Fractal
	{
	private:
		glm::ivec2 resolution;
		Viewport viewport;
		std::int32_t oversampling;

		glm::ivec2 size;
		GLuint currentIteration;

		RAIIWrapper<GLuint> textureValues;
		RAIIWrapper<GLuint> textureIterations;
		RAIIWrapper<GLuint> textureColor;

		RAIIWrapper<GLuint> framebuffer;
		RAIIWrapper<GLuint> programClear;
		RAIIWrapper<GLuint> programIterate;
		GLint locationBound;
		GLint locationCurrentIteration;
		GLint locationSize;
		GLint locationViewport;
		GLint locationIterationsPerFrame;

		RAIIWrapper<GLuint> textureValuesBuffered;
		RAIIWrapper<GLuint> textureIterationsBuffered;
		RAIIWrapper<GLuint> textureColorBuffered;

		RAIIWrapper<GLuint> framebufferBuffered;

		RAIIWrapper<GLuint> programRender;
		GLint locationResolution;

		RAIIWrapper<GLuint> programUpdate;
		GLint locationSizeUpdate;
		GLint locationSizeOldUpdate;
		GLint locationViewportUpdate;
		GLint locationViewportOldUpdate;

		static inline RAIIWrapper<GLuint> createTextureValues(const glm::ivec2& size)
		{
			RAIIWrapper<GLuint> textureValues(glCreate(Texture)(), glDelete(Texture));

			glBindTexture(GL_TEXTURE_2D, textureValues);
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, size.x, size.y, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, nullptr);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			return textureValues;
		}

		static inline RAIIWrapper<GLuint> createTextureIterations(const glm::ivec2& size)
		{
			RAIIWrapper<GLuint> textureIterations(glCreate(Texture)(), glDelete(Texture));

			glBindTexture(GL_TEXTURE_2D, textureIterations);
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, size.x, size.y, 0, GL_RG_INTEGER, GL_UNSIGNED_BYTE, nullptr);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			
			return textureIterations;
		}

		static inline RAIIWrapper<GLuint> createTextureColor(const glm::ivec2& size)
		{
			RAIIWrapper<GLuint> textureColor = RAIIWrapper<GLuint>(glCreate(Texture)(), glDelete(Texture));

			glBindTexture(GL_TEXTURE_2D, textureColor);
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			return textureColor;
		}

		static inline RAIIWrapper<GLuint> createFramebuffer(const RAIIWrapper<GLuint>& textureValues, const RAIIWrapper<GLuint>& textureIterations, const RAIIWrapper<GLuint>& textureColor)
		{
			RAIIWrapper<GLuint> framebuffer(glCreate(Framebuffer)(), glDelete(Framebuffer));

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureValues, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, textureIterations, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, textureColor, 0);

			GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

			glDrawBuffers(3, drawBuffers);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				throw std::runtime_error("GL-Error: Framebuffer not completed.");
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return framebuffer;
		}

		void initialize(const glm::ivec2& resolution, const std::int32_t oversampling)
		{
			this->resolution = resolution;
			this->oversampling = oversampling;

			this->size = resolution * oversampling;

			this->textureValues = this->createTextureValues(this->size);

			this->textureIterations = this->createTextureIterations(this->size);

			this->textureColor = this->createTextureColor(this->size);

			this->framebuffer = this->createFramebuffer(this->textureValues, this->textureIterations, this->textureColor);

			this->reset();
		}

		void update(const glm::ivec2& resolution, const Viewport& viewport, const std::int32_t oversampling)
		{
			glm::ivec2 size = resolution * oversampling;

			RAIIWrapper<GLuint> textureValues = this->textureValuesBuffered;

			RAIIWrapper<GLuint> textureIterations = this->textureIterationsBuffered;

			RAIIWrapper<GLuint> textureColor = this->textureColorBuffered;

			RAIIWrapper<GLuint> framebuffer = this->framebufferBuffered;

			bool valid = textureValues && textureIterations && textureColor && framebuffer;

			if (size != this->size || !valid)
			{
				textureValues = this->createTextureValues(size);

				textureIterations = this->createTextureIterations(size);

				textureColor = this->createTextureColor(size);

				framebuffer = this->createFramebuffer(textureValues, textureIterations, textureColor);

				this->textureValuesBuffered = nullptr;

				this->textureIterationsBuffered = nullptr;

				this->textureColorBuffered = nullptr;

				this->framebufferBuffered = nullptr;
			}
			
			if (size == this->size)
			{
				this->textureValuesBuffered = this->textureValues;

				this->textureIterationsBuffered = this->textureIterations;

				this->textureColorBuffered = this->textureColor;

				this->framebufferBuffered = this->framebuffer;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

			glViewport(0, 0, size.x, size.y);

			glUseProgram(this->programUpdate);

			glUniform2iv(this->locationSizeUpdate, 1, reinterpret_cast<const GLint*>(&size));
			glUniform2iv(this->locationSizeOldUpdate, 1, reinterpret_cast<const GLint*>(&this->size));
			glUniform4dv(this->locationViewportUpdate, 1, reinterpret_cast<const GLdouble*>(&viewport));
			glUniform4dv(this->locationViewportOldUpdate, 1, reinterpret_cast<const GLdouble*>(&this->viewport));

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, this->textureIterations);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->textureValues);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glBindTexture(GL_TEXTURE_2D, textureColor);

			glGenerateMipmap(GL_TEXTURE_2D);

			this->currentIteration = 0;

			this->textureValues = textureValues;

			this->textureIterations = textureIterations;

			this->textureColor = textureColor;

			this->framebuffer = framebuffer;

			this->resolution = resolution;
			this->viewport = viewport;
			this->oversampling = oversampling;

			this->size = size;
		}

	public:
		Mandelbrot(const glm::ivec2& resolution, const Viewport& viewport, const std::int32_t oversampling = 2, const std::int32_t iterationsPerFrame = 25) :
			resolution(resolution), viewport(viewport), oversampling(oversampling)
		{
			auto vertexShaderCode = CODE(\
				#version 420 core \n\

				vec2 vertices[4] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));
				int indices[6] = int[](0, 1, 2, 1, 2, 3);

				void main()
				{
					gl_Position = vec4(vertices[indices[gl_VertexID]] * 2.0 - vec2(1.0), 0.0, 1.0);
				}
			);

			gl::requireExtension("GL_ARB_gpu_shader_fp64");

			auto fragmentShaderCode = CODE(\
				#version 420 core \n\
				#extension GL_ARB_gpu_shader_fp64 : enable \n\

				precision highp float;

				layout(location = 0) out uvec4 value;
				layout(location = 1) out uvec4 iterations;
				layout(location = 2) out vec4 color;

				void main()
				{
					value = uvec4(0);
					iterations = uvec4(0);
					color = vec4(0.0, 0.0, 0.0, 1.0);
				}
			);

			this->programClear = gl::compileAndLinkShaders(vertexShaderCode, fragmentShaderCode);


			fragmentShaderCode = CODE(\
				#version 420 core \n\
				#extension GL_ARB_gpu_shader_fp64 : enable \n\

				precision highp float;

				layout(binding = 0) uniform usampler2D samplerValues;
				layout(binding = 1) uniform usampler2D samplerIterations;

				uniform double bound;
				uniform uint currentIteration;

				uniform ivec2 size;
				uniform dvec4 viewport;

				uniform int iterationsPerFrame;

				layout(location = 0) out uvec4 value;
				layout(location = 1) out uvec4 iterations;
				layout(location = 2) out vec4 color;

				void main()
				{
					ivec2 pixel = ivec2(gl_FragCoord.xy);

					uvec4 oldValue = texelFetch(samplerValues, pixel, 0);

					dvec2 z = dvec2(packDouble2x32(oldValue.xy), packDouble2x32(oldValue.zw));

					iterations = texelFetch(samplerIterations, pixel, 0);

					color = vec4(0.0, 0.0, 0.0, 1.0);

					dvec2 c = mix(viewport.xz, viewport.yw, dvec2(gl_FragCoord.xy) / size);

					if (z.x != 1.0 / 0.0)
					{
						for (int i = 0; i < iterationsPerFrame; i++)
						{
							z = dvec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;

							iterations.r++;

							if (z.x * z.x + z.y * z.y > bound * bound)
							{
								z.x = 1.0 / 0.0;

								break;
							}
						}
					}

					if (z.x == 1.0 / 0.0)
					{
						color.r = pow(sin(float(iterations.r) / 10.0), 2.0);
					}
					else if (currentIteration <= iterations.g)
					{
						color.r = pow(sin(float(iterations.g) / 10.0), 2.0);
					}

					value.xy = unpackDouble2x32(z.x);
					value.zw = unpackDouble2x32(z.y);
				}
			);

			this->programIterate = gl::compileAndLinkShaders(vertexShaderCode, fragmentShaderCode);

			this->locationBound = glGetUniformLocation(this->programIterate, "bound");
			this->locationCurrentIteration = glGetUniformLocation(this->programIterate, "currentIteration");

			this->locationSize = glGetUniformLocation(this->programIterate, "size");
			this->locationViewport = glGetUniformLocation(this->programIterate, "viewport");

			this->locationIterationsPerFrame = glGetUniformLocation(this->programIterate, "iterationsPerFrame");


			fragmentShaderCode = CODE(\
				#version 420 core \n\

				precision highp float;

				uniform sampler2D sampler;

				uniform ivec2 resolution;

				out vec4 color;

				void main()
				{
					vec2 screen = gl_FragCoord.xy / resolution;

					color = texture(sampler, screen);
				}
			);

			this->programRender = gl::compileAndLinkShaders(vertexShaderCode, fragmentShaderCode);

			this->locationResolution = glGetUniformLocation(this->programRender, "resolution");

			fragmentShaderCode = CODE(\
				#version 420 core \n\
				#extension GL_ARB_gpu_shader_fp64 : enable \n\

				precision highp float;
				
				layout(binding = 0) uniform usampler2D samplerValues;
				layout(binding = 1) uniform usampler2D samplerIterations;

				uniform ivec2 size;
				uniform ivec2 sizeOld;

				uniform dvec4 viewport;
				uniform dvec4 viewportOld;

				layout(location = 0) out uvec4 value;
				layout(location = 1) out uvec4 iterations;
				layout(location = 2) out vec4 color;

				void main()
				{
					dvec2 screen = dvec2(gl_FragCoord.xy) / size;

					dvec2 pos = mix(viewport.xz, viewport.yw, screen);

					dvec2 screenOld = (pos - viewportOld.xz) / (viewportOld.yw - viewportOld.xz);

					dvec2 fragCoordOld = screenOld * sizeOld;

					ivec2 pixel = ivec2(fragCoordOld);

					uvec4 iterationsOld = texelFetch(samplerIterations, pixel, 0);

					uvec4 oldValue = texelFetch(samplerValues, pixel, 0);

					dvec2 z = dvec2(packDouble2x32(oldValue.xy), packDouble2x32(oldValue.zw));

					value = uvec4(0);
					iterations = uvec4(0, 0, 0, 0);
					color = vec4(0.0, 0.0, 0.0, 1.0);

					if (z.x == 1.0 / 0.0)
					{
						iterations.g = iterationsOld.r;
					}
					else if (iterationsOld.r < iterationsOld.g)
					{
						iterations.g = iterationsOld.g;
					}
				}
			);

			this->programUpdate = gl::compileAndLinkShaders(vertexShaderCode, fragmentShaderCode);

			this->locationSizeUpdate = glGetUniformLocation(this->programUpdate, "size");
			this->locationSizeOldUpdate = glGetUniformLocation(this->programUpdate, "sizeOld");

			this->locationViewportUpdate = glGetUniformLocation(this->programUpdate, "viewport");
			this->locationViewportOldUpdate = glGetUniformLocation(this->programUpdate, "viewportOld");

			this->initialize(resolution, oversampling);
		}

		virtual void reset() override
		{
			glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);

			glViewport(0, 0, this->size.x, this->size.y);

			glUseProgram(this->programClear);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, this->textureIterations);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->textureValues);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glBindTexture(GL_TEXTURE_2D, this->textureColor);

			glGenerateMipmap(GL_TEXTURE_2D);

			this->currentIteration = 0;
		}

		virtual void iterate(const std::int32_t iterations) override
		{
			glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);

			glViewport(0, 0, this->size.x, this->size.y);

			glUseProgram(this->programIterate);

			glUniform1d(this->locationBound, 2.0);
			glUniform1ui(this->locationCurrentIteration, this->currentIteration);

			glUniform2iv(this->locationSize, 1, reinterpret_cast<const GLint*>(&this->size));
			glUniform4dv(this->locationViewport, 1, reinterpret_cast<const GLdouble*>(&this->viewport));

			glUniform1i(this->locationIterationsPerFrame, iterations);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, this->textureIterations);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->textureValues);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			this->currentIteration += iterations;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glBindTexture(GL_TEXTURE_2D, this->textureColor);

			glGenerateMipmap(GL_TEXTURE_2D);
		}

		virtual void render(const glm::ivec2 resolution, const Viewport& viewport) override
		{
			glUseProgram(this->programRender);

			glUniform2iv(this->locationResolution, 1, reinterpret_cast<const GLint*>(&resolution));

			glBindTexture(GL_TEXTURE_2D, this->textureColor);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			if (resolution != this->resolution || viewport.viewport != this->viewport.viewport)
			{
				this->update(resolution, viewport, this->oversampling);

				glViewport(0, 0, resolution.x, resolution.y);
			}
		}

		virtual Viewport getPreferredViewport() const override
		{
			return Viewport(-2.0f, 1.0f, -1.0f, 1.0f);
		}

		virtual std::int32_t getPreferredIterationsPerFrame() const override
		{
			return 10;
		}

		virtual void options()
		{
			std::int32_t oversampling = this->oversampling;

			ImGui::SliderInt("Oversampling", &oversampling, 1, 16);

			if (this->oversampling != oversampling)
			{
				this->update(this->resolution, this->viewport, oversampling);
			}
		}
	};
}
