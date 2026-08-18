///-------------------------------------------------------------------------------------------------
/// 
/// \file GenericVRPNServer.hpp
/// \author Jozef Legény
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include <map>
#include <vector>
#include <string>

class vrpn_Connection;
class vrpn_Button_Server;
class vrpn_Analog_Server;

///<summary> A class providing a very simple generic VRPN server capable of creating Analog and Button controls. </summary>
class CGenericVrpnServer
{
public:
    struct SButtonServer
    {
        vrpn_Button_Server* server;
        int nButton;
        std::vector<int> cache;
    };

    struct SAnalogServer
    {
        vrpn_Analog_Server* server;
        int nChannel;
    };

    /// Public singleton factory
    static CGenericVrpnServer* GetInstance(const int port);

    static void DeleteInstance();

    /// Public destructor
    ~CGenericVrpnServer();

    /// The loop() method has to be called periodically in order for vrpn to work
    void Loop();

    /** Creates a new button object within the VRPN server
     * \param name name of the vrpn peripheral
     * \param buttonCount number of virtual buttons in the peripeheral
     */
    void AddButton(const std::string& name, const int buttonCount);

    /** Change the button state of a button inside a created VRPN peripheral
     * \param name name of the vrpn peripheral containing the button
     * \param index index of the button (beginning by 0)
     * \param state new state of the button 0 = off, 1 = on
     */
    void ChangeButtonState(const std::string& name, const int index, const int state);


    /** Get the state of a button
     * \param name name of the vrpn peripheral containing the button
     * \param index index of the button (beginning by 0)
     * \return the state of the button
     */
    int GetButtonState(const std::string& name, const int index);

    /** Creates a new analog object within the VRPN server
     * \param name name of the vrpn peripheral
     * \param nChannel number of channels in the peripeheral
     */
    void AddAnalog(const std::string& name, const int nChannel);

    /** Change the state of channels of an analog VRPN peripheral
     * \param name name of the vrpn peripheral containing the analog control
     * \param ... :  list of the values (double)
     */
    void ChangeAnalogState(std::string name, ...);


    /** Gets a pointer to the channel array
     * \param name name of the vrpn peripheral containing the analog control
     * \return pointer to the array containing the channels
     */
    double* GetAnalogChannels(const std::string& name);

    /** Marks the selected analog server channels as modified so the values are sent in the next loop
     * \param name name of the vrpn peripheral containing the analog control
     */
    void ReportAnalogChanges(const std::string& name);

protected:
    static CGenericVrpnServer* m_serverInstance;

    explicit CGenericVrpnServer(const int port);

    vrpn_Connection* m_connection = nullptr;
    std::map<std::string, SButtonServer> m_buttonServer;
    std::map<std::string, SAnalogServer> m_analogServer;
};
