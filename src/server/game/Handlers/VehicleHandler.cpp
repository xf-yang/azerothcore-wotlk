/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Log.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "Vehicle.h"
#include "WorldPacket.h"
#include "WorldSession.h"


//解除控制车辆
void WorldSession::HandleDismissControlledVehicle(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Recvd CMSG_DISMISS_CONTROLLED_VEHICLE");

    ObjectGuid vehicleGUID = _player->GetCharmGUID();

    if (!vehicleGUID)                                       // something wrong here...
    {
        recvData.rfinish();                                // prevent warnings spam
        return;
    }

    ObjectGuid guid;
    recvData >> guid.ReadAsPacked();

    // pussywizard: typical check for incomming movement packets
    if (!_player->m_mover || !_player->m_mover->IsInWorld() || _player->m_mover->IsDuringRemoveFromWorld() || guid != _player->m_mover->GetGUID())
    {
        recvData.rfinish(); // prevent warnings spam
        _player->ExitVehicle();
        return;
    }

    MovementInfo mi;
    mi.guid = guid;
    ReadMovementInfo(recvData, &mi);

    _player->m_mover->m_movementInfo = mi;

    _player->ExitVehicle();
}

// HandleChangeSeatsOnControlledVehicle .在受控车辆上更换座位
void WorldSession::HandleChangeSeatsOnControlledVehicle(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Recvd CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE");

    GetPlayer()->Say("<WorldSession.HandleChangeSeatsOnControlledVehicle>",LANG_UNIVERSAL);

    {//mylog
        GetPlayer()->Say("<WorldSession.HandleChangeSeatsOnControlledVehicle.1. />",LANG_UNIVERSAL);
    }

    Unit* vehicle_base = GetPlayer()->GetVehicleBase();
    if (!vehicle_base)
    {
        // std::string msg = "WorldSession.HandleChange... 2:" ;  
        // GetPlayer()->Say(msg,LANG_UNIVERSAL);
        recvData.rfinish();                                // prevent warnings spam
        return;
    }

    // GetPlayer()->Say("WorldSession.HandleChange... 2",LANG_UNIVERSAL);

    VehicleSeatEntry const* seat = GetPlayer()->GetVehicle()->GetSeatForPassenger(GetPlayer());
    if (!seat->CanSwitchFromSeat())
    {
        recvData.rfinish();                                // prevent warnings spam

        // std::string msg = "WorldSession.HandleChange... 3:";  
        // GetPlayer()->Say(msg,LANG_UNIVERSAL);

        LOG_ERROR("network.opcode", "HandleChangeSeatsOnControlledVehicle, Opcode: {}, Player {} tried to switch seats but current seatflags {} don't permit that.",
                       recvData.GetOpcode(), GetPlayer()->GetGUID().ToString(), seat->m_flags);
        return;
    }

    // std::string msg8 = "WorldSession.HandleChange... 3. Opcode:" + std::to_string( recvData.GetOpcode()) ;  
    // GetPlayer()->Say(msg8,LANG_UNIVERSAL);

    switch (recvData.GetOpcode())
    {
        case CMSG_REQUEST_VEHICLE_PREV_SEAT:
            // GetPlayer()->Say("WorldSession.HandleChange... CMSG_REQUEST_VEHICLE_PREV_SEAT. ChangeSeat(-1, false) ",LANG_UNIVERSAL); 
            GetPlayer()->ChangeSeat(-1, false);
            break;
        case CMSG_REQUEST_VEHICLE_NEXT_SEAT:
            // GetPlayer()->Say("WorldSession.HandleChange... CMSG_REQUEST_VEHICLE_NEXT_SEAT. ChangeSeat(-1, true) ",LANG_UNIVERSAL);   
            GetPlayer()->ChangeSeat(-1, true);
            break;
        case CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE:
            {
                
                // GetPlayer()->Say("WorldSession.HandleChange... 31",LANG_UNIVERSAL);

                ObjectGuid guid;        // current vehicle guid
                recvData >> guid.ReadAsPacked();
                // GetPlayer()->Say("WorldSession.HandleChange... 32",LANG_UNIVERSAL);

                // pussywizard:
                if (vehicle_base->GetGUID() != guid)
                {
                    recvData.rfinish(); // prevent warnings spam
                    return;
                }
                // GetPlayer()->Say("WorldSession.HandleChange... 33",LANG_UNIVERSAL);

                MovementInfo movementInfo;
                movementInfo.guid = guid;
                ReadMovementInfo(recvData, &movementInfo);
                vehicle_base->m_movementInfo = movementInfo;

                ObjectGuid accessory;        //  accessory guid
                recvData >> accessory.ReadAsPacked();

                int8 seatId;
                recvData >> seatId;

                {//mylog
                    std::string msg = Acore::StringFormatFmt(
                        "<WorldSession.HandleChangeSeatsOnControlledVehicle.3.4. seatId='{}' accessory='{}' />"
                        ,seatId
                        ,accessory ? "t" : "f"
                    );
                    GetPlayer()->Say(msg,LANG_UNIVERSAL);
                }

                if (!accessory){
                    // GetPlayer()->Say("WorldSession.HandleChange... 35 CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE. ChangeSeat(-1, seatId) ",LANG_UNIVERSAL); 
                    GetPlayer()->ChangeSeat(-1, seatId > 0); // prev/next
                }
                else if (Unit* vehUnit = ObjectAccessor::GetUnit(*GetPlayer(), accessory))
                {
                    {//mylog
                        std::string msg = Acore::StringFormatFmt(
                            " <WorldSession.HandleChangeSeatsOnControlledVehicle.3.6. seatId='{}' />"
                            ,seatId
                        );
                        GetPlayer()->Say(msg,LANG_UNIVERSAL);
                    }

                    //如果这个座位是载具附件上的，

                    if (Vehicle* vehicle = vehUnit->GetVehicleKit()){
                        // GetPlayer()->Say("WorldSession.HandleChange... 362",LANG_UNIVERSAL);
                        if (vehicle->HasEmptySeat(seatId)){
                            
                            {//mylog
                                std::string msg = Acore::StringFormatFmt(
                                    " <WorldSession.HandleChangeSeatsOnControlledVehicle.3.6. vehUnit='{}' VehicleKit='{}' seatId='{}' />"
                                    ,vehUnit->GetName()
                                    ,vehicle->GetBase()->GetName()
                                    ,seatId
                                );
                                GetPlayer()->Say(msg,LANG_UNIVERSAL);
                            }

                            vehUnit->HandleSpellClick(GetPlayer(), seatId); //某些载具切换座位的时候不是同一个载具，例如 攻城坦克和攻城炮台
                            //  GetPlayer()->ChangeSeat(seatId, seatId > 0); // prev/next //试试这个
                        }else{
                            // std::string msg364 = Acore::StringFormatFmt("WorldSession.HandleChange... 364.Seat is not Empty, seatId:{}; "
                            //     ,seatId
                            // );
                            // vehUnit->Say(msg364,LANG_UNIVERSAL);
                        }
                    }
                }
                break;
            }
        case CMSG_REQUEST_VEHICLE_SWITCH_SEAT:
            {
                ObjectGuid guid;        // current vehicle guid
                recvData >> guid.ReadAsPacked();

                int8 seatId;
                recvData >> seatId;

                if (vehicle_base->GetGUID() == guid){
                    // GetPlayer()->Say("WorldSession.HandleChange... CMSG_REQUEST_VEHICLE_SWITCH_SEAT. ChangeSeat(seatId) ",LANG_UNIVERSAL);
                    GetPlayer()->ChangeSeat(seatId);
                 } else if (Unit* vehUnit = ObjectAccessor::GetUnit(*GetPlayer(), guid)){
                    if (Vehicle* vehicle = vehUnit->GetVehicleKit()){
                        if (vehicle->HasEmptySeat(seatId)){
                            vehUnit->HandleSpellClick(GetPlayer(), seatId);
                        } else {
                            // std::string msg364 = Acore::StringFormatFmt("WorldSession.HandleChange... 365. Seat is not Empty, seatId:{}; "
                            //     ,seatId
                            // );
                            // vehUnit->Say(msg364,LANG_UNIVERSAL);
                        }
                    }
                 }
                break;
            }
        default:
            // std::string msg2 = "WorldSession.HandleChange... UNKNOW OperateCode!! Opcode:" + std::to_string( recvData.GetOpcode()) ;  
            // GetPlayer()->Say(msg2,LANG_UNIVERSAL);
            break;
    }

    GetPlayer()->Say("</WorldSession.HandleChangeSeatsOnControlledVehicle >",LANG_UNIVERSAL);
}

// HandleEnterPlayerVehicle 进入玩家车辆
void WorldSession::HandleEnterPlayerVehicle(WorldPacket& data)
{
    GetPlayer()->Say("<WorldSession.HandleEnterPlayerVehicle>",LANG_UNIVERSAL);

    // Read guid
    ObjectGuid guid;
    data >> guid;

    if (Player* player = ObjectAccessor::GetPlayer(*_player, guid))
    {
        if (!player->GetVehicleKit())
            return;
        if (!player->IsInRaidWith(_player))
            return;
        if (!player->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
            return;
        // Xinef:
        if (!_player->FindMap() || _player->FindMap()->IsBattleArena())
            return;

        _player->EnterVehicle(player);
    }
    GetPlayer()->Say("</WorldSession.HandleEnterPlayerVehicle>",LANG_UNIVERSAL);
}

//驱逐乘客
void WorldSession::HandleEjectPassenger(WorldPacket& data)
{
    Vehicle* vehicle = _player->GetVehicleKit();
    if (!vehicle)
    {
        data.rfinish();                                     // prevent warnings spam
        LOG_ERROR("network.opcode", "HandleEjectPassenger: Player {} is not in a vehicle!", GetPlayer()->GetGUID().ToString());
        return;
    }

    ObjectGuid guid;
    data >> guid;

    if (guid.IsPlayer())
    {
        Player* player = ObjectAccessor::GetPlayer(*_player, guid);
        if (!player)
        {
            LOG_ERROR("network.opcode", "Player {} tried to eject player {} from vehicle, but the latter was not found in world!", GetPlayer()->GetGUID().ToString(), guid.ToString());
            return;
        }

        if (!player->IsOnVehicle(vehicle->GetBase()))
        {
            LOG_ERROR("network.opcode", "Player {} tried to eject player {}, but they are not in the same vehicle", GetPlayer()->GetGUID().ToString(), guid.ToString());
            return;
        }

        VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(player);
        ASSERT(seat);
        if (seat->IsEjectable())
            player->ExitVehicle();
        else
            LOG_ERROR("network.opcode", "Player {} attempted to eject player {} from non-ejectable seat.", GetPlayer()->GetGUID().ToString(), guid.ToString());
    }
    else if (guid.IsCreature())
    {
        Unit* unit = ObjectAccessor::GetUnit(*_player, guid);
        if (!unit) // creatures can be ejected too from player mounts
        {
            LOG_ERROR("network.opcode", "Player {} tried to eject creature guid {} from vehicle, but the latter was not found in world!", GetPlayer()->GetGUID().ToString(), guid.ToString());
            return;
        }

        if (!unit->IsOnVehicle(vehicle->GetBase()))
        {
            LOG_ERROR("network.opcode", "Player {} tried to eject unit {}, but they are not in the same vehicle", GetPlayer()->GetGUID().ToString(), guid.ToString());
            return;
        }

        VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(unit);
        ASSERT(seat);
        if (seat->IsEjectable())
        {
            ASSERT(GetPlayer() == vehicle->GetBase());
            unit->ExitVehicle();
        }
        else
            LOG_ERROR("network.opcode", "Player {} attempted to eject creature {} from non-ejectable seat.", GetPlayer()->GetGUID().ToString(), guid.ToString());
    }
    else
        LOG_ERROR("network.opcode", "HandleEjectPassenger: Player {} tried to eject invalid {}", GetPlayer()->GetGUID().ToString(), guid.ToString());
}

//请求出口
void WorldSession::HandleRequestVehicleExit(WorldPacket& /*recvData*/)
{
    LOG_DEBUG("network", "WORLD: Recvd CMSG_REQUEST_VEHICLE_EXIT");

    if (Vehicle* vehicle = GetPlayer()->GetVehicle())
    {
        if (VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(GetPlayer()))
        {
            if (seat->CanEnterOrExit())
                GetPlayer()->ExitVehicle();
            else
                LOG_ERROR("network.opcode", "Player {} tried to exit vehicle, but seatflags {} (ID: {}) don't permit that.",
                               GetPlayer()->GetGUID().ToString(), seat->m_ID, seat->m_flags);
        }
    }
}
