#include "stdafx.h"
#include "SceneManager.h"
#include "AppContext.h"
#include "TinyXml/tinyxml2.h"
#include "Shapes/Line.h"
#include "Shapes/Pen.h"
#include "Shapes/Rectangle.h"
#include "Shapes/Ellipse.h"

using namespace paint;
using namespace tinyxml2;

/////////////////////////////////////////////////////

SceneManager::SceneManager()
{
	InitShapesFactory();
}

/////////////////////////////////////////////////////

SceneManager::~SceneManager()
{
	for (auto shape : m_shapes)
	{
		delete shape;
	}

	for (auto shape : m_undoBuffer)
	{
		delete shape;
	}
}

/////////////////////////////////////////////////////

void SceneManager::InitShapesFactory()
{
	m_shapesFactory.insert(std::pair<DWORD, IFactory*>(Tool::Pen, new Factory<Pen>));
	m_shapesFactory.insert(std::pair<DWORD, IFactory*>(Tool::Line, new Factory<Line>));
	m_shapesFactory.insert(std::pair<DWORD, IFactory*>(Tool::Rectangle, new Factory<Rectangle>));
	m_shapesFactory.insert(std::pair<DWORD, IFactory*>(Tool::Ellipse, new Factory<Ellipse>));
}

/////////////////////////////////////////////////////

void SceneManager::NotifyCreationFinished()
{
	auto context = AppContext::GetInstance();
	context->GetRenderer()->Refresh();

	m_undoBuffer.clear();

	UpdateHistoryButtons();
}

/////////////////////////////////////////////////////

void SceneManager::NotifyCreationStateChanged()
{
	auto context = AppContext::GetInstance();
	auto currentShape = context->GetCurrentTool()->GetShape();

	auto shapesCount = m_shapes.size();
	if ((shapesCount == 0) || (currentShape != m_shapes[shapesCount - 1]))
	{
		m_shapes.push_back(currentShape);
		m_sceneHasChanges = true;
	}	

	AppContext::GetInstance()->GetRenderer()->Refresh();
	UpdateHistoryButtons();
}

/////////////////////////////////////////////////////

void SceneManager::UndoAction()
{
	auto shapesCount = m_shapes.size();
	if (shapesCount > 0)
	{
		auto lastShape = m_shapes[shapesCount - 1];
		m_shapes.pop_back();

		// Add to undo queue
		m_undoBuffer.push_back(lastShape);

		AppContext::GetInstance()->GetRenderer()->Refresh();
	}

	UpdateHistoryButtons();
}

/////////////////////////////////////////////////////

void SceneManager::RedoAction()
{
	auto undoBufferSize = m_undoBuffer.size();
	if (undoBufferSize > 0)
	{
		auto lastShape = m_undoBuffer[undoBufferSize - 1];
		m_undoBuffer.pop_back();

		// Put back to scene
		m_shapes.push_back(lastShape);

		AppContext::GetInstance()->GetRenderer()->Refresh();
	}

	UpdateHistoryButtons();
}

/////////////////////////////////////////////////////

void SceneManager::SetButtonsId(DWORD undoButtonId, DWORD redoButtonId, DWORD saveButtonId)
{
	m_undoButtonId = undoButtonId;
	m_redoButtonId = redoButtonId;
	m_saveButtonId = saveButtonId;
}

/////////////////////////////////////////////////////

void SceneManager::UpdateHistoryButtons()
{
	HWND hwnd = AppContext::GetInstance()->GetWindowHandle();
	HMENU menu = GetMenu(hwnd);
	
	EnableMenuItem(menu, m_undoButtonId, m_shapes.size() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(menu, m_redoButtonId, m_undoBuffer.size() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(menu, m_saveButtonId, m_sceneHasChanges ? MF_ENABLED : MF_GRAYED);
}

/////////////////////////////////////////////////////

void SceneManager::SaveToEnhancedMetafile(const char* path)
{
	XMLDocument doc;
	auto root = doc.NewElement("Scene");
	doc.InsertEndChild(root);

	for (auto shape : m_shapes)
	{
		auto shapeElement = shape->ToXml(&doc);
		root->InsertEndChild(shapeElement);
	}

	XMLError error = doc.SaveFile(path);
	if (error)
	{
		MessageBoxExA(AppContext::GetInstance()->GetWindowHandle(),
			"Failed to save file", "IO Error", MB_ICONERROR, 0);
	}
}

/////////////////////////////////////////////////////

void SceneManager::LoadFromEnhancedMetafile(const char* path)
{
	Clear();

	auto context = AppContext::GetInstance();

	XMLDocument doc;
	XMLError error = doc.LoadFile(path);

	// If file has not been parsed, throw message box error and terminate
	if (error)
	{
		MessageBoxExA(context->GetWindowHandle(), "Can't load file", "IO Error", MB_ICONERROR, 0);
		return;
	}
	
	auto root = doc.RootElement();
	for (auto child = root->FirstChild(); child; child = child->NextSibling())
	{
		auto element = child->ToElement();
		const char* name = element->Value();
		Tool tool = context->GetToolbar()->GetToolByName(name);

		IFactory* shapeFactory = m_shapesFactory[tool.value()];
		if (shapeFactory)
		{
			Shape* shape = static_cast<Shape*>(shapeFactory->Create());

			if (shape)
			{	
				shape->FromXml(element);	// Deserialize
				m_shapes.push_back(shape);	// Add to scene
			}
		}	
	}

	m_sceneHasChanges = false;
	UpdateHistoryButtons();

	context->GetRenderer()->Refresh();
}

/////////////////////////////////////////////////////

void SceneManager::Clear()
{
	m_shapes.clear();
	m_undoBuffer.clear();

	auto context = AppContext::GetInstance();
	SceneManager::GetInstance()->UpdateHistoryButtons();
	context->GetRenderer()->Refresh();
}

/////////////////////////////////////////////////////
