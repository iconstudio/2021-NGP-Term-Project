#include "pch.h"


void Render::transform_set(HDC world, XFORM& info) {
	SetWorldTransform(world, &info);
}

void Render::transform_set_identity(HDC world) {
	SetWorldTransform(world, &transform_identity);
}

void Render::draw_end(HDC canvas, HGDIOBJ object_old, HGDIOBJ object_new) {		
	SelectObject(canvas, object_old);
	DeleteObject(object_new);
}

void Render::draw_clear(HDC canvas, int width, int height, COLORREF color) {
	auto m_hPen = CreatePen(PS_NULL, 1, color);
	auto m_oldhPen = static_cast<HPEN>(SelectObject(canvas, m_hPen));
	auto m_hBR = CreateSolidBrush(color);
	auto m_oldhBR = static_cast<HBRUSH>(SelectObject(canvas, m_hBR));
	draw_rectangle(canvas, 0, 0, width, height);
	draw_end(canvas, m_oldhBR, m_hBR);
	draw_end(canvas, m_oldhPen, m_hPen);
}

BOOL Render::draw_rectangle(HDC canvas, int x1, int y1, int x2, int y2) {
	return Rectangle(canvas, x1, y1, x2, y2);
}
