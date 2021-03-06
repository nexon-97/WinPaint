#include "stdafx.h"
#include "Renderer.h"
#include "AppContext.h"
#include "SceneManager.h"
#include "Renderers/PenRenderer.h"
#include "Renderers/LineRenderer.h"
#include "Renderers/RectangleRenderer.h"
#include "Renderers/EllipseRenderer.h"
#include "Renderers/PolylineRenderer.h"
#include "Renderers/TextRenderer.h"

using namespace paint;

/////////////////////////////////////////////////////

Renderer::Renderer(HWND hWnd)
	: m_hwnd(hWnd)
{
	Init();
}

/////////////////////////////////////////////////////

void Renderer::Render()
{
	PAINTSTRUCT ps;
	m_screenContext = BeginPaint(m_hwnd, &ps);

	HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(m_offscreenHdc, m_offscreenBitmap));

	// Fill backbuffer with system color
	HBRUSH hbrBkGnd = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	FillRect(m_offscreenHdc, &m_wndClientRect, hbrBkGnd);
	DeleteObject(hbrBkGnd);

	// Render the scene

	auto shapes = SceneManager::GetInstance()->GetShapes();
	for (auto shape : shapes)
	{
		RenderShape(shape);
	}

	// Read client size
	int width = m_wndClientRect.right - m_wndClientRect.left;
	int height = m_wndClientRect.bottom - m_wndClientRect.top;

	// Swap buffers
	BitBlt(m_screenContext, m_wndClientRect.left, m_wndClientRect.top, width, height, m_offscreenHdc, 0, 0, SRCCOPY);
	SelectObject(m_offscreenHdc, oldBitmap);

	EndPaint(m_hwnd, &ps);
}

/////////////////////////////////////////////////////

void Renderer::RenderShape(Shape* shape)
{
	auto tool = shape->GetTool();
	auto shapeRenderer = GetShapeRenderer(tool);

	RefreshShapeStyle(shape);
	if (shapeRenderer)
	{
		shapeRenderer->Render(shape);
	}
}

/////////////////////////////////////////////////////

void Renderer::Init()
{
	PAINTSTRUCT ps;
	m_screenContext = BeginPaint(m_hwnd, &ps);

	GetClientRect(m_hwnd, &m_wndClientRect);
	int width = m_wndClientRect.right - m_wndClientRect.left;
	int height = m_wndClientRect.bottom - m_wndClientRect.top;

	InitBackbuffer(m_screenContext, width, height);

	EndPaint(m_hwnd, &ps);

	InitShapeRenderers();
}

/////////////////////////////////////////////////////

void Renderer::InitShapeRenderers()
{
	m_shapeRenderers.insert(std::pair<int, IRenderer*>(Tool::Pen, new PenRenderer()));
	m_shapeRenderers.insert(std::pair<int, IRenderer*>(Tool::Line, new LineRenderer()));
	m_shapeRenderers.insert(std::pair<int, IRenderer*>(Tool::Rectangle, new RectangleRenderer()));
	m_shapeRenderers.insert(std::pair<int, IRenderer*>(Tool::Ellipse, new EllipseRenderer()));
	m_shapeRenderers.insert(std::pair<int, IRenderer*>(Tool::Polyline, new PolylineRenderer()));
	m_shapeRenderers.insert(std::pair<int, IRenderer*>(Tool::Text, new TextRenderer()));
}

/////////////////////////////////////////////////////

void Renderer::Resize(Point size)
{
	Init();
	Refresh();
}

/////////////////////////////////////////////////////

void Renderer::InitBackbuffer(HDC context, int width, int height)
{
	m_offscreenHdc = CreateCompatibleDC(context);
	m_offscreenBitmap = CreateCompatibleBitmap(context, width, height);
}

/////////////////////////////////////////////////////

void Renderer::Refresh()
{
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

/////////////////////////////////////////////////////

BITMAP Renderer::GetImage()
{
	BITMAP bitmap;
	GetObject(m_offscreenBitmap, sizeof(BITMAP), &bitmap);

	return bitmap;
}

/////////////////////////////////////////////////////

IRenderer* Renderer::GetShapeRenderer(Tool tool) const
{
	auto it = m_shapeRenderers.find(tool.value());
	if (it != m_shapeRenderers.end())
	{
		return it->second;
	}

	return nullptr;
}

/////////////////////////////////////////////////////

void Renderer::DrawLine(Point from, Point to)
{
	MoveToEx(m_offscreenHdc, from.x, from.y, NULL);
	LineTo(m_offscreenHdc, to.x, to.y);
}

/////////////////////////////////////////////////////

void Renderer::DrawRect(Point from, Point to, bool hollow)
{
	if (hollow)
	{
		SelectObject(m_offscreenHdc, GetStockObject(HOLLOW_BRUSH));
	}
	
	Rectangle(m_offscreenHdc, from.x, from.y, to.x, to.y);
}

/////////////////////////////////////////////////////

void Renderer::DrawEllipse(Point from, Point to, bool hollow)
{
	if (hollow)
	{
		SelectObject(m_offscreenHdc, GetStockObject(HOLLOW_BRUSH));
	}
	
	Ellipse(m_offscreenHdc, from.x, from.y, to.x, to.y);
}

/////////////////////////////////////////////////////

void Renderer::DrawText(Point position, LPCSTR text, DWORD length, Font* font, COLORREF color)
{
	font->Apply(m_offscreenHdc);			// Select font	
	SetTextColor(m_offscreenHdc, color);	// Select text color

	TextOutA(m_offscreenHdc, position.x, position.y, text, length);
}

/////////////////////////////////////////////////////

void Renderer::RefreshShapeStyle(Shape* shape)
{
	COLORREF penColor = shape->GetPenColor();
	COLORREF brushColor = shape->GetBrushColor();
	DWORD penThickness = shape->GetLineThickness();

	HPEN pen = CreatePen(PS_SOLID, penThickness, penColor);

	//SelectObject(m_offscreenHdc, GetStockObject(DC_PEN));
	SelectObject(m_offscreenHdc, pen);
	SetDCPenColor(m_offscreenHdc, penColor);
	SelectObject(m_offscreenHdc, GetStockObject(DC_BRUSH));
	SetDCBrushColor(m_offscreenHdc, brushColor);
}

/////////////////////////////////////////////////////

Point Renderer::GetSize()
{
	return Point(m_wndClientRect.right - m_wndClientRect.left, m_wndClientRect.bottom - m_wndClientRect.top);
}

/////////////////////////////////////////////////////
