#pragma once
#include "ogl2Defs.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			constexpr const GLchar* VERTEX_SHADER =
				"#version 120\n"

				"uniform mat4 projectionMatrix;\n"

				"attribute vec2 pos;\n"
				"attribute vec4 col;\n"
				"attribute vec2 uv;\n"

				"varying vec4 colOut;\n"
				"varying vec2 uvOut;\n"

				"void main() {\n"
				"    colOut = col;\n"
				"    uvOut = uv;\n"
				"    gl_Position = projectionMatrix * vec4(pos.xy,0,1);\n"
				"}\n";

			constexpr const GLchar* FRAGMENT_SHADER =
				"#version 120\n"

				"uniform sampler2D texSampler;\n"

				"varying vec4 colOut;\n"
				"varying vec2 uvOut;\n"

				"void main() {\n"
				"    gl_FragColor = colOut * texture2D(texSampler, uvOut);\n"
				"}\n";
		}

	}

}