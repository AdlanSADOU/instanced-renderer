#pragma once

struct VertexData {
	float x, y, z, w;
	float r, g, b, a;
};

VertexData vertex_Data[] = {
  {
	-0.7f, -0.7f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 0.0f
  },
  {
	-0.7f, 0.7f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 0.0f
  },
  {
	0.7f, -0.7f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 0.0f
  },
  {
	0.7f, 0.7f, 0.0f, 1.0f,
	0.3f, 0.3f, 0.3f, 0.0f
  }
};

VertexData my_quad[] = {
  {
	-0.1f, -0.1f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 0.0f
  },
  {
	-0.2f, 0.2f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 0.0f
  },
  {
	0.7f, -0.1f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 0.0f
  },
  {
	0.7f, 0.7f, 0.0f, 1.0f,
	0.3f, 0.3f, 0.3f, 0.0f
  }
};
