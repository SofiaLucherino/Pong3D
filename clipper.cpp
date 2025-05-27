
clip_vertex CalculateIntersection(clip_vertex Start, clip_vertex End, clip_axis Axis)
{
	clip_vertex Result = {};

	f32 S = 0.0f;

	switch (Axis)
	{		        case ClipAxis_Left:
				{
					S = -(Start.Pos.w + Start.Pos.x) / ((End.Pos.x - Start.Pos.x) + (End.Pos.w - Start.Pos.w));
				} break;

				case ClipAxis_Right:
				{
					S = (Start.Pos.w - Start.Pos.x) / ((End.Pos.x - Start.Pos.x) - (End.Pos.w - Start.Pos.w));
				} break;

				case ClipAxis_Top:
				{
					S = (Start.Pos.w - Start.Pos.y) / ((End.Pos.y - Start.Pos.y) - (End.Pos.w - Start.Pos.w));
				} break;

				case ClipAxis_Bottom:
				{
					S = -(Start.Pos.w + Start.Pos.y) / ((End.Pos.y - Start.Pos.y) + (End.Pos.w - Start.Pos.w));
				} break;

				case ClipAxis_Near:
				{
					S = -Start.Pos.z / (End.Pos.z - Start.Pos.z);
				} break;

				case ClipAxis_Far:
				{
					S = (Start.Pos.w - Start.Pos.z) / ((End.Pos.z - Start.Pos.z) - (End.Pos.w - Start.Pos.w));
				} break;

				case ClipAxis_W:
				{
					S = (W_CLIPPING_VALUE - Start.Pos.w) / (End.Pos.w - Start.Pos.w);
				} break;

				default:
				{
					InvalidCodePath;
				} break;
}
	/* {
		case ClipAxis_Left:
		{
			S = -(Start.Pos.w + Start.Pos.x) / ((End.Pos.x - Start.Pos.x) + (End.Pos.w - Start.Pos.w));
		} break;

		case ClipAxis_Right:
		{
			S = (Start.Pos.w - Start.Pos.x) / ((End.Pos.x - Start.Pos.x) - (End.Pos.w - Start.Pos.w));
		} break;
	
		case ClipAxis_Top:
		{
			S = (Start.Pos.w - Start.Pos.y) / ((End.Pos.y - Start.Pos.y) - (End.Pos.w - Start.Pos.w));
		} break;
	
		case ClipAxis_Bottom:
		{
			S = -(Start.Pos.w + Start.Pos.y) / ((End.Pos.y - Start.Pos.y) + (End.Pos.w - Start.Pos.w));
		} break;
			
		case ClipAxis_Near:
		{
			S = -(Start.Pos.z) / (End.Pos.z - Start.Pos.z);
		} break;

		case ClipAxis_Far:
		{
			S = (Start.Pos.w - Start.Pos.z) / ((End.Pos.z - Start.Pos.z) - (End.Pos.w - Start.Pos.w));
		} break;

		case ClipAxis_W:
		{
			S = (W_CLIPPING_VALUE - Start.Pos.w) / (End.Pos.w - Start.Pos.w);
		} break;

		default:
		{
			InvalidCodePath;
		} break;
	}*/

	Result.Pos = ((1.0f - S) * Start.Pos) + (S * End.Pos);
	Result.UV = ((1.0f - S) * Start.UV) + (S * End.UV);

	return Result;
}

b32 IsBehindPlane(clip_vertex Vertex, clip_axis Axis)
{
	b32 Result = false;
	
	switch (Axis)
	{
		case ClipAxis_Left: 
		{
			Result = Vertex.Pos.x < -Vertex.Pos.w;
			} break;

		case ClipAxis_Right: 
		{
			Result = Vertex.Pos.x > Vertex.Pos.w;
			} break;

		case ClipAxis_Top: 
		{
			Result = Vertex.Pos.y > Vertex.Pos.w;
			} break;

		case ClipAxis_Bottom: 
		{
			Result = Vertex.Pos.y < -Vertex.Pos.w;
			} break;

		case ClipAxis_Near: 
		{
			Result = Vertex.Pos.z < 0.0f;
			} break;

		case ClipAxis_Far: 
		{
			Result = Vertex.Pos.z > Vertex.Pos.w;
			} break;

		case ClipAxis_W: 
		{
			Result = Vertex.Pos.w < W_CLIPPING_VALUE;
			} break;

		default: 
		{
			InvalidCodePath;
		} break;
	}

	return Result;
}

void ClipPolygonToAxis(clip_result* Input, clip_result* Output, clip_axis Axis) 
{
	Output->NumTriangles = 0;

	for (u32 TriangleId = { 0 }; TriangleId < Input->NumTriangles; ++TriangleId)
	{
		u32 VertexId[3] =
		{
			3 * TriangleId + 0,
			3 * TriangleId + 1,
			3 * TriangleId + 2
		};

		clip_vertex Vertices[] =
		{
			Input->Vertices[VertexId[0]],
			Input->Vertices[VertexId[1]],
			Input->Vertices[VertexId[2]]
		};

		b32 BehindPlane[3] =
		{
			IsBehindPlane(Vertices[0], Axis),
			IsBehindPlane(Vertices[1], Axis),
			IsBehindPlane(Vertices[2], Axis)
		};

		u32 NumBehindPlane = BehindPlane[0] + BehindPlane[1] + BehindPlane[2];
		switch (NumBehindPlane) 
		{
			case 0: 
			{
				//Adding full Triangle to Output
				u32 CurrTriangleId = Output->NumTriangles++;
				Output->Vertices[3 * CurrTriangleId + 0] = Vertices[0];
				Output->Vertices[3 * CurrTriangleId + 1] = Vertices[1];
				Output->Vertices[3 * CurrTriangleId + 2] = Vertices[2];
			} break;

			case 1: 
			{
				// Adding two Triangles that are parts of previous Triangle to output
				u32 CurrTriangleId = Output->NumTriangles;
				Output->NumTriangles += 2;
				u32 CurrVertexId = 0;
				b32 IsTriangleAdded = false;

				for (u32 EdgeId = { 0 }; EdgeId < 3; ++EdgeId)
				{
					u32 StartVertexId = EdgeId;
					u32 EndVertexId = EdgeId == 2 ? 0 : (EdgeId + 1);

					clip_vertex StartVertex = Vertices[StartVertexId];
					clip_vertex EndVertex = Vertices[EndVertexId];

					b32 StartBehindPlane = BehindPlane[StartVertexId];
					b32 EndBehindPlane = BehindPlane[EndVertexId];

					if (!StartBehindPlane)
					{
						Output->Vertices[3 * CurrTriangleId + CurrVertexId++] = StartVertex;
					}

					if (!IsTriangleAdded && CurrVertexId == 3)
					{
						IsTriangleAdded = true;
						CurrTriangleId += 1;
						CurrVertexId = 0;

						Output->Vertices[3 * CurrTriangleId + CurrVertexId++] = Output->Vertices[3 * (CurrTriangleId - 1) + 0];
						Output->Vertices[3 * CurrTriangleId + CurrVertexId++] = Output->Vertices[3 * (CurrTriangleId - 1) + 2];
					}

					if (StartBehindPlane == !EndBehindPlane)
					{
						Output->Vertices[3 * CurrTriangleId + CurrVertexId++] = CalculateIntersection(StartVertex, EndVertex, Axis);
					}

					if (!IsTriangleAdded && CurrVertexId == 3)
					{
						IsTriangleAdded = true;
						CurrTriangleId += 1;
						CurrVertexId = 0;

						Output->Vertices[3 * CurrTriangleId + CurrVertexId++] = Output->Vertices[3 * (CurrTriangleId - 1) + 0];
						Output->Vertices[3 * CurrTriangleId + CurrVertexId++] = Output->Vertices[3 * (CurrTriangleId - 1) + 2];
					}
				}
			} break;

			case 2: 
			{
				// Adding cutted Triangle to Output
				u32 CurrTriangleId = Output->NumTriangles++;
				u32 CurrVertexId = 0;

				for (u32 EdgeId = { 0 }; EdgeId < 3; ++EdgeId) 
				{
					u32 StartVertexId = EdgeId;
					u32 EndVertexId = EdgeId == 2 ? 0 : (EdgeId + 1);

					clip_vertex StartVertex = Vertices[StartVertexId];
					clip_vertex EndVertex = Vertices[EndVertexId];

					b32 StartBehindPlane = BehindPlane[StartVertexId];
					b32 EndBehindPlane = BehindPlane[EndVertexId];

					if (!StartBehindPlane) 
					{
						Output->Vertices[3 * CurrTriangleId + CurrVertexId++] = StartVertex;
					}

					if (StartBehindPlane == !EndBehindPlane)
					{
						Output->Vertices[3 * CurrTriangleId + CurrVertexId++] = CalculateIntersection(StartVertex, EndVertex, Axis);
					}
				}
			} break;

			case 3: 
			{
				//We're not adding a Triangle to Output
			} break;

			default: 
			{
				InvalidCodePath;
			} break;
		}
	}
}