#pragma once

//
// Scaleform removed.
//
// This file is now a compatibility no-op layer.
// It exists only so old UI/HUD code can still compile while we migrate UI to RmlUI.
//

#ifndef WARZ_SCALEFORM_REMOVED
#define WARZ_SCALEFORM_REMOVED 1
#endif

#include <stddef.h>
#include <stdint.h>

namespace Scaleform
{
	namespace GFx
	{
		class Movie
		{
		public:
			Movie() {}
			~Movie() {}
		};

		class Value
		{
		public:
			enum ValueType
			{
				VT_Undefined,
				VT_Null,
				VT_Boolean,
				VT_Number,
				VT_String,
				VT_StringW,
				VT_Object,
				VT_Array,
				VT_DisplayObject
			};

			class DisplayInfo
			{
			public:
				DisplayInfo()
					: X(0.0f)
					, Y(0.0f)
					, XScale(100.0f)
					, YScale(100.0f)
					, Alpha(100.0f)
					, Visible(true)
				{
				}

				void SetPosition(float x, float y)
				{
					X = x;
					Y = y;
				}

				void SetX(float x)
				{
					X = x;
				}

				void SetY(float y)
				{
					Y = y;
				}

				void SetScale(float xscale, float yscale)
				{
					XScale = xscale;
					YScale = yscale;
				}

				void SetXScale(float xscale)
				{
					XScale = xscale;
				}

				void SetYScale(float yscale)
				{
					YScale = yscale;
				}

				void SetAlpha(float alpha)
				{
					Alpha = alpha;
				}

				void SetVisible(bool visible)
				{
					Visible = visible;
				}

				float GetX() const { return X; }
				float GetY() const { return Y; }
				float GetXScale() const { return XScale; }
				float GetYScale() const { return YScale; }
				float GetAlpha() const { return Alpha; }
				bool GetVisible() const { return Visible; }

			private:
				float X;
				float Y;
				float XScale;
				float YScale;
				float Alpha;
				bool Visible;
			};

		public:
			Value()
				: Type(VT_Undefined)
				, NumberValue(0.0)
				, BoolValue(false)
				, StringValue("")
				, WStringValue(L"")
			{
			}

			Value(ValueType type)
				: Type(type)
				, NumberValue(0.0)
				, BoolValue(false)
				, StringValue("")
				, WStringValue(L"")
			{
			}

			Value(bool value)
				: Type(VT_Boolean)
				, NumberValue(value ? 1.0 : 0.0)
				, BoolValue(value)
				, StringValue("")
				, WStringValue(L"")
			{
			}

			Value(int value)
				: Type(VT_Number)
				, NumberValue((double)value)
				, BoolValue(value != 0)
				, StringValue("")
				, WStringValue(L"")
			{
			}

			Value(unsigned int value)
				: Type(VT_Number)
				, NumberValue((double)value)
				, BoolValue(value != 0)
				, StringValue("")
				, WStringValue(L"")
			{
			}

			Value(float value)
				: Type(VT_Number)
				, NumberValue((double)value)
				, BoolValue(value != 0.0f)
				, StringValue("")
				, WStringValue(L"")
			{
			}

			Value(double value)
				: Type(VT_Number)
				, NumberValue(value)
				, BoolValue(value != 0.0)
				, StringValue("")
				, WStringValue(L"")
			{
			}

			Value(const char* value)
				: Type(VT_String)
				, NumberValue(0.0)
				, BoolValue(false)
				, StringValue(value ? value : "")
				, WStringValue(L"")
			{
			}

			Value(const wchar_t* value)
				: Type(VT_StringW)
				, NumberValue(0.0)
				, BoolValue(false)
				, StringValue("")
				, WStringValue(value ? value : L"")
			{
			}

			void SetUndefined()
			{
				Type = VT_Undefined;
				NumberValue = 0.0;
				BoolValue = false;
				StringValue = "";
				WStringValue = L"";
			}

			void SetNull()
			{
				Type = VT_Null;
				NumberValue = 0.0;
				BoolValue = false;
				StringValue = "";
				WStringValue = L"";
			}

			void SetBoolean(bool value)
			{
				Type = VT_Boolean;
				BoolValue = value;
				NumberValue = value ? 1.0 : 0.0;
			}

			void SetBool(bool value)
			{
				SetBoolean(value);
			}

			void SetNumber(double value)
			{
				Type = VT_Number;
				NumberValue = value;
				BoolValue = value != 0.0;
			}

			void SetInt(int value)
			{
				SetNumber((double)value);
			}

			void SetUInt(unsigned int value)
			{
				SetNumber((double)value);
			}

			void SetString(const char* value)
			{
				Type = VT_String;
				StringValue = value ? value : "";
			}

			void SetStringW(const wchar_t* value)
			{
				Type = VT_StringW;
				WStringValue = value ? value : L"";
			}

			void SetObject()
			{
				Type = VT_Object;
			}

			void SetArray()
			{
				Type = VT_Array;
			}

			void SetDisplayInfo(const DisplayInfo& info)
			{
				Display = info;
				Type = VT_DisplayObject;
			}

			bool GetDisplayInfo(DisplayInfo* info) const
			{
				if(info)
					*info = Display;

				return false;
			}

			ValueType GetType() const
			{
				return Type;
			}

			bool IsUndefined() const { return Type == VT_Undefined; }
			bool IsNull() const { return Type == VT_Null; }
			bool IsBool() const { return Type == VT_Boolean; }
			bool IsBoolean() const { return Type == VT_Boolean; }
			bool IsNumber() const { return Type == VT_Number; }
			bool IsString() const { return Type == VT_String || Type == VT_StringW; }
			bool IsObject() const { return Type == VT_Object || Type == VT_DisplayObject; }
			bool IsArray() const { return Type == VT_Array; }

			bool GetBool() const
			{
				return BoolValue;
			}

			bool GetBoolean() const
			{
				return BoolValue;
			}

			double GetNumber() const
			{
				return NumberValue;
			}

			int GetInt() const
			{
				return (int)NumberValue;
			}

			unsigned int GetUInt() const
			{
				return (unsigned int)NumberValue;
			}

			const char* GetString() const
			{
				return StringValue ? StringValue : "";
			}

			const wchar_t* GetStringW() const
			{
				return WStringValue ? WStringValue : L"";
			}

			bool SetMember(const char* name, const Value& value)
			{
				(void)name;
				(void)value;
				return false;
			}

			bool SetMember(const char* name, const char* value)
			{
				(void)name;
				(void)value;
				return false;
			}

			bool SetMember(const char* name, const wchar_t* value)
			{
				(void)name;
				(void)value;
				return false;
			}

			bool SetMember(const char* name, int value)
			{
				(void)name;
				(void)value;
				return false;
			}

			bool SetMember(const char* name, unsigned int value)
			{
				(void)name;
				(void)value;
				return false;
			}

			bool SetMember(const char* name, float value)
			{
				(void)name;
				(void)value;
				return false;
			}

			bool SetMember(const char* name, double value)
			{
				(void)name;
				(void)value;
				return false;
			}

			bool SetMember(const char* name, bool value)
			{
				(void)name;
				(void)value;
				return false;
			}

			bool GetMember(const char* name, Value* value) const
			{
				(void)name;

				if(value)
					value->SetUndefined();

				return false;
			}

			bool HasMember(const char* name) const
			{
				(void)name;
				return false;
			}

			bool DeleteMember(const char* name)
			{
				(void)name;
				return false;
			}

			bool SetElement(unsigned int index, const Value& value)
			{
				(void)index;
				(void)value;
				return false;
			}

			bool GetElement(unsigned int index, Value* value) const
			{
				(void)index;

				if(value)
					value->SetUndefined();

				return false;
			}

			bool PushBack(const Value& value)
			{
				(void)value;
				return false;
			}

			bool PushBack(int value)
			{
				(void)value;
				return false;
			}

			bool PushBack(unsigned int value)
			{
				(void)value;
				return false;
			}

			bool PushBack(float value)
			{
				(void)value;
				return false;
			}

			bool PushBack(double value)
			{
				(void)value;
				return false;
			}

			bool PushBack(bool value)
			{
				(void)value;
				return false;
			}

			bool PushBack(const char* value)
			{
				(void)value;
				return false;
			}

			bool PushBack(const wchar_t* value)
			{
				(void)value;
				return false;
			}

			void SetArraySize(unsigned int size)
			{
				(void)size;
				Type = VT_Array;
			}

			unsigned int GetArraySize() const
			{
				return 0;
			}

		private:
			ValueType Type;
			double NumberValue;
			bool BoolValue;
			const char* StringValue;
			const wchar_t* WStringValue;
			DisplayInfo Display;
		};
	}
}

class r3dScaleformMovie
{
public:
	typedef Scaleform::GFx::Value GFxValue;

	class IEventCallback
	{
	public:
		virtual ~IEventCallback() {}
	};

	template <typename T>
	class TGFxEICallback : public IEventCallback
	{
	public:
		typedef void (T::*CallbackWithMovie)(r3dScaleformMovie*, const Scaleform::GFx::Value*, unsigned int);
		typedef void (T::*CallbackNoMovie)(const Scaleform::GFx::Value*, unsigned int);

		TGFxEICallback(T* object, CallbackWithMovie callback)
		{
			(void)object;
			(void)callback;
		}

		TGFxEICallback(T* object, CallbackNoMovie callback)
		{
			(void)object;
			(void)callback;
		}
	};

public:
	r3dScaleformMovie()
		: Loaded(false)
		, KeyboardCapture(false)
		, MouseCapture(false)
	{
	}

	~r3dScaleformMovie()
	{
		Unload();
	}

	bool Load(const char* movieName)
	{
		(void)movieName;
		Loaded = false;
		return false;
	}

	bool Load(const char* movieName, bool captureKeyboard)
	{
		(void)movieName;
		KeyboardCapture = captureKeyboard;
		Loaded = false;
		return false;
	}

	bool Load(const char* movieName, bool captureKeyboard, bool captureMouse)
	{
		(void)movieName;
		KeyboardCapture = captureKeyboard;
		MouseCapture = captureMouse;
		Loaded = false;
		return false;
	}

	void Unload()
	{
		Loaded = false;
	}

	bool IsLoaded() const
	{
		return false;
	}

	bool IsMovieLoaded() const
	{
		return false;
	}

	void Update()
	{
	}

	void Update(float dt)
	{
		(void)dt;
	}

	void Draw()
	{
	}

	void UpdateAndDraw()
	{
	}

	void UpdateAndDraw(float dt)
	{
		(void)dt;
	}

	void SetKeyboardCapture(bool enabled)
	{
		KeyboardCapture = enabled;
	}

	void SetMouseCapture(bool enabled)
	{
		MouseCapture = enabled;
	}

	void SetBackBufferViewport()
	{
	}

	void SetBackBufferViewport(int x, int y, int width, int height)
	{
		(void)x;
		(void)y;
		(void)width;
		(void)height;
	}

	void SetCurentRTViewport()
	{
	}

	void SetCurrentRTViewport()
	{
	}

	void SetViewport(int x, int y, int width, int height)
	{
		(void)x;
		(void)y;
		(void)width;
		(void)height;
	}

	bool Invoke(const char* methodName)
	{
		(void)methodName;
		return false;
	}

	bool Invoke(const char* methodName, const Scaleform::GFx::Value* args, unsigned int argCount)
	{
		(void)methodName;
		(void)args;
		(void)argCount;
		return false;
	}

	bool Invoke(const char* methodName, const Scaleform::GFx::Value* args, unsigned int argCount, Scaleform::GFx::Value* result)
	{
		(void)methodName;
		(void)args;
		(void)argCount;

		if(result)
			result->SetUndefined();

		return false;
	}

	bool Invoke(const char* methodName, const Scaleform::GFx::Value& value)
	{
		(void)methodName;
		(void)value;
		return false;
	}

	bool Invoke(const char* methodName, const char* value)
	{
		(void)methodName;
		(void)value;
		return false;
	}

	bool Invoke(const char* methodName, const wchar_t* value)
	{
		(void)methodName;
		(void)value;
		return false;
	}

	bool Invoke(const char* methodName, int value)
	{
		(void)methodName;
		(void)value;
		return false;
	}

	bool Invoke(const char* methodName, unsigned int value)
	{
		(void)methodName;
		(void)value;
		return false;
	}

	bool Invoke(const char* methodName, float value)
	{
		(void)methodName;
		(void)value;
		return false;
	}

	bool Invoke(const char* methodName, double value)
	{
		(void)methodName;
		(void)value;
		return false;
	}

	bool Invoke(const char* methodName, bool value)
	{
		(void)methodName;
		(void)value;
		return false;
	}

	bool SetVariable(const char* path, const Scaleform::GFx::Value& value)
	{
		(void)path;
		(void)value;
		return false;
	}

	bool SetVariable(const char* path, const char* value)
	{
		(void)path;
		(void)value;
		return false;
	}

	bool SetVariable(const char* path, const wchar_t* value)
	{
		(void)path;
		(void)value;
		return false;
	}

	bool SetVariable(const char* path, int value)
	{
		(void)path;
		(void)value;
		return false;
	}

	bool SetVariable(const char* path, unsigned int value)
	{
		(void)path;
		(void)value;
		return false;
	}

	bool SetVariable(const char* path, float value)
	{
		(void)path;
		(void)value;
		return false;
	}

	bool SetVariable(const char* path, double value)
	{
		(void)path;
		(void)value;
		return false;
	}

	bool SetVariable(const char* path, bool value)
	{
		(void)path;
		(void)value;
		return false;
	}

	bool GetVariable(Scaleform::GFx::Value* value, const char* path) const
	{
		(void)path;

		if(value)
			value->SetUndefined();

		return false;
	}

	bool SetVariableArray(const char* path, unsigned int index, const Scaleform::GFx::Value* values, unsigned int count)
	{
		(void)path;
		(void)index;
		(void)values;
		(void)count;
		return false;
	}

	bool GetVariableArray(const char* path, unsigned int index, Scaleform::GFx::Value* values, unsigned int count) const
	{
		(void)path;
		(void)index;
		(void)values;
		(void)count;
		return false;
	}

	bool CreateObject(Scaleform::GFx::Value* value)
	{
		if(value)
			value->SetObject();

		return false;
	}

	bool CreateObject(Scaleform::GFx::Value* value, const char* className)
	{
		(void)className;

		if(value)
			value->SetObject();

		return false;
	}

	bool CreateObject(Scaleform::GFx::Value* value, const char* className, const Scaleform::GFx::Value* args, unsigned int argCount)
	{
		(void)className;
		(void)args;
		(void)argCount;

		if(value)
			value->SetObject();

		return false;
	}

	bool CreateArray(Scaleform::GFx::Value* value)
	{
		if(value)
			value->SetArray();

		return false;
	}

	void CreateString(Scaleform::GFx::Value* value, const char* text)
	{
		if(value)
			value->SetString(text);
	}

	void CreateStringW(Scaleform::GFx::Value* value, const wchar_t* text)
	{
		if(value)
			value->SetStringW(text);
	}

	void SetExternalInterfaceRetVal(const Scaleform::GFx::Value& value)
	{
		(void)value;
	}

	void SetExternalInterfaceRetVal(bool value)
	{
		(void)value;
	}

	void SetExternalInterfaceRetVal(int value)
	{
		(void)value;
	}

	void SetExternalInterfaceRetVal(unsigned int value)
	{
		(void)value;
	}

	void SetExternalInterfaceRetVal(float value)
	{
		(void)value;
	}

	void SetExternalInterfaceRetVal(double value)
	{
		(void)value;
	}

	void SetExternalInterfaceRetVal(const char* value)
	{
		(void)value;
	}

	void SetExternalInterfaceRetVal(const wchar_t* value)
	{
		(void)value;
	}

	template <typename T>
	void RegisterEventHandler(const char* eventName, T* object, void (T::*callback)(r3dScaleformMovie*, const Scaleform::GFx::Value*, unsigned int))
	{
		(void)eventName;
		(void)object;
		(void)callback;
	}

	template <typename T>
	void RegisterEventHandler(const char* eventName, T* object, void (T::*callback)(const Scaleform::GFx::Value*, unsigned int))
	{
		(void)eventName;
		(void)object;
		(void)callback;
	}

	void RegisterEventHandler(const char* eventName, IEventCallback* callback)
	{
		(void)eventName;

		if(callback)
			delete callback;
	}

	Scaleform::GFx::Movie* GetMovie()
	{
		return 0;
	}

	const Scaleform::GFx::Movie* GetMovie() const
	{
		return 0;
	}

private:
	bool Loaded;
	bool KeyboardCapture;
	bool MouseCapture;
};