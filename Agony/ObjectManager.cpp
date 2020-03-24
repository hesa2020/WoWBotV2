#include "ObjectManager.h"
#include "Offsets.h"
#include "CGUnit.h"
#include "Game.h"
#include <iostream>

namespace Agony
{
	namespace Native
	{
		std::vector<CGObject*> ObjectManager::GetVisibleObjects()
		{
			auto player = Agony::Native::Game::Me();
			auto name = reinterpret_cast<const char* (__fastcall*)(WoWObject*)>(Offsets::Base + Offsets::CGUnit_C__GetUnitNameExposed)(player);
			//std::cout << "Obj name " << unit->GetName() << std::endl;
			std::cout << "Player name " << name << std::endl;

			auto playerGuid = GetGUIDFromToken("player");
			std::cout << "Player guid address 0x" << std::hex << playerGuid << std::endl;

			std::vector<CGObject*> returnList;
			const CGObjectManager* m_CurObjectMgr = *reinterpret_cast<CGObjectManager**>(Offsets::Base + Offsets::ObjectMgr);
			for (auto i = *(uintptr_t*)(m_CurObjectMgr->VisibleObjects.Next); i != m_CurObjectMgr->VisibleObjects.Next; i = *(uintptr_t*)(i))
			{
				CGObject* wowObj = reinterpret_cast<CGObject*>(i - 0x18);
				if (wowObj->GetType() == WoWObjectType::Player)
				{
					CGUnit* unit = reinterpret_cast<CGUnit*>(wowObj);
					std::cout << "Obj player address 0x" << std::hex << unit << std::endl;
					std::cout << "Obj position = " << unit->GetPosition().x << ", " << unit->GetPosition().y << ", " << unit->GetPosition().z << std::endl;
					std::cout << "Obj race " << std::dec << (int)unit->GetRace() << std::endl;
					std::cout << "Obj class " << std::dec << (int)unit->GetClass() << std::endl;
					std::cout << "Obj HP " << std::dec << unit->GetCurrentHP() << "/" << std::dec << unit->GetMaxHP() << std::endl;
					std::cout << "Obj Mana " << std::dec << unit->GetCurrentMana() << "/" << std::dec << unit->GetMaxMana() << std::endl;
					std::cout << "Obj name " << unit->GetName() << std::endl;
				}
				else
				{
					std::cout << "Obj name " << wowObj->GetName() << std::endl;
				}
				returnList.push_back(wowObj);
			}
			return returnList;
		}

		CGObject* ObjectManager::GetBaseFromToken(std::string token)
		{
			return reinterpret_cast<CGObject * (__fastcall*)(const char*)>(Offsets::Base + Offsets::GetBaseFromToken)(token.c_str());
		}

		ObjectGuid* ObjectManager::GetGUIDFromToken(std::string token)
		{
			ObjectGuid objectGUID{};
			reinterpret_cast<bool (__fastcall*)(ObjectGuid*, const char*)>(Offsets::Base + Offsets::Script_GetGUIDFromToken)(&objectGUID, token.c_str());
			return &objectGUID;
		}

		CGObject* ObjectManager::GetObjectFromGuid(ObjectGuid* guid)
		{
			/*const CGObjectManager* m_CurObjectMgr = *reinterpret_cast<CGObjectManager**>(Offsets::Base + Offsets::ObjectMgr);
			uint64_t arrayIndex = (uint64_t)(m_CurObjectMgr->ActiveObjects.Capacity - 1) & (0xA2AA033B * guid->LoWord + 0xD6D018F5 * guid->HiWord);
			uint64_t entryAddress = *reinterpret_cast<uint64_t*>(m_CurObjectMgr->ActiveObjects.Array + 8 * arrayIndex);
			while (entryAddress > 0)
			{
				CurMgr0x8Entry entry = *reinterpret_cast<CurMgr0x8Entry*>(entryAddress);
				ObjectHeader header = Bot.Mem64.Read<ObjectHeader>(entry.ObjectBase);
				if (header.WowGuid == guid)
				{
					/*WowData data = new WowData
					{
						Address = entry.ObjectBase,
						Header = header,
					};* /
					return entry.ObjectBase;
				}
				entryAddress = entry.Next;
			}*/
			return nullptr;
		}

		bool ObjectManager::UnitExist(std::string token)
		{
			return GetBaseFromToken(token) != nullptr;
		}
	}
}