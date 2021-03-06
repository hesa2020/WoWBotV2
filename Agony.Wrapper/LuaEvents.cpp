#include "LuaEvents.h"
#include "../Agony/LuaFunctions.h"
#include <msclr\marshal_cppstd.h>

static Agony::WoWInternals::LuaEvents::LuaEvents()
{
	ATTACH_DOMAIN();
	ATTACH_EVENT(LuaEvent, Agony::Native::Game::Game::GetInstance()->OnLuaEvent, void(std::string, std::vector<std::any>));
}

void Agony::WoWInternals::LuaEvents::DomainUnloadEventHandler(System::Object^, System::EventArgs^)
{
	DETACH_EVENT(LuaEvent, Agony::Native::Game::Game::GetInstance()->OnLuaEvent, void(std::string, std::vector<std::any>));
}

bool Agony::WoWInternals::LuaEvents::AttachEvent(System::String^ eventName, LuaEventHandlerDelegate^ handler)
{
	if (!RegisteredEvents->ContainsKey(eventName))
	{
		if (Agony::Native::LuaFunctions::RegisterEvent(msclr::interop::marshal_as<std::string>(eventName)))
		{
			RegisteredEvents->Add(eventName, gcnew System::Collections::Generic::List<LuaEventHandlerDelegate^>);
		}
		else
		{
			return false;
		}
	}
	if (!RegisteredEvents[eventName]->Contains(handler))
	{
		RegisteredEvents[eventName]->Add(handler);
	}
	return true;
}

void Agony::WoWInternals::LuaEvents::DetachEvent(System::String^ eventName, LuaEventHandlerDelegate^ handler)
{
	if (RegisteredEvents->ContainsKey(eventName))
	{
		RegisteredEvents[eventName]->Remove(handler);
		if (RegisteredEvents[eventName]->Count == 0)
		{
			Agony::Native::LuaFunctions::UnregisterEvent(msclr::interop::marshal_as<std::string>(eventName));
		}
	}
}

void Agony::WoWInternals::LuaEvents::OnLuaEventNative(std::string eventName, std::vector<std::any> results)
{
	START_TRACE
		auto eventNameManaged = gcnew System::String(eventName.c_str());
		if (Agony::WoWInternals::LuaEvents::RegisteredEvents->ContainsKey(eventNameManaged))
		{
			auto sender = gcnew System::Object();
			array<System::Object^>^ args = gcnew array<System::Object^>(results.size());
			int i = 0;
			//Convert things to Managed types
			for (auto arg : results)
			{
				if (arg.type() == typeid(std::string))
				{
					args[i] = gcnew System::String(std::any_cast<std::string>(arg).c_str());
				}
				else if (arg.type() == typeid(double))
				{
					args[i] = gcnew System::Double(std::any_cast<double>(arg));
				}
				else if (arg.type() == typeid(int))
				{
					args[i] = gcnew System::Int32(std::any_cast<int>(arg));
				}
				else if (arg.type() == typeid(bool))
				{
					args[i] = gcnew System::Boolean(std::any_cast<bool>(arg));
				}
				else if (arg.type() == typeid(uintptr_t))
				{
					args[i] = gcnew System::UIntPtr(std::any_cast<uintptr_t>(arg));
				}
				else if (arg.type() == typeid(std::nullptr_t))
				{
					args[i] = (System::Object^)nullptr;
				}
				i++;
			}
			//LuaEventArgs ( for c# )
			auto eventArgs = gcnew Agony::WoWInternals::LuaEventArgs(eventNameManaged, 0, args);
			//Trigger inside plugins...
			for each (auto eventHandle in Agony::WoWInternals::LuaEvents::RegisteredEvents[eventArgs->EventName]->ToArray())
			{
				START_TRACE
					eventHandle(sender, eventArgs);
				END_TRACE
			}
		}
	END_TRACE
}