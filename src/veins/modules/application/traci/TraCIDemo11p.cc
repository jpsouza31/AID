//
// Copyright (C) 2006-2011 Christoph Sommer <christoph.sommer@uibk.ac.at>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "veins/modules/application/traci/TraCIDemo11p.h"

#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <vector>

using namespace veins;

Define_Module(veins::TraCIDemo11p);

TraCIDemo11pMessage* TraCIDemo11p::createMessage (int to, int senderId, std::string dado, simtime_t beginTime = NULL) {
    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
    populateWSM(wsm, -1);
    wsm->setSerial(to);
    wsm->setSenderAddress(senderId);
    wsm->setDemoData(dado.c_str());
    if (beginTime == NULL)
        wsm->setBeginTime(simTime());
    else
        wsm->setBeginTime(beginTime);
    return wsm;
}

void TraCIDemo11p::createAndSendMessage (int to, int senderId, simtime_t beginTime = NULL) {
    std::string dado = "Mensagem para o node " + std::to_string(to);
    TraCIDemo11pMessage* wsm = createMessage(to, myId, dado, beginTime);
    sendDown(wsm);
//    std::cout << "## Enviado a mensagem para o " << to << endl;
}

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
        c = 0;
        s = 0;
        t = 0;
        messageSendInterval = 20;
        timeToSendMessage = simTime() + messageSendInterval;
        rsuIds[0] = 16;
        rsuIds[1] = 26;
        rsuIds[2] = 21;
        rsuIds[3] = 31;
        rsuIds[4] = 41;
        rsuIds[5] = 36;
        rsuIds[6] = 46;
    }
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa)
{
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);
//    std::cout << "##" << wsm->getDemoData() << ", sou o node : " << myId << " no tempo: " << simTime() << endl;
    if ( myId == wsm->getSerial() ) {
        std::cout << "######### MENSAGEM RECEBIDA - Sou o node " << myId << endl;
    } else {
        if (t == 0) {
            c = 0;
            s = 0;
            t = simTime();

            TraCIDemo11pMessage* newWsm = createMessage(wsm->getSerial(), myId, wsm->getDemoData(), wsm->getBeginTime());
            scheduleAt(simTime() + 0.1, newWsm);
        } else {
            c++;
            simtime_t t1 = simTime();
            l.push_back(t1 - t);
            t = t1;
        }
    }
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
//    std::cout << "## Entrei aqui e o valor de c ficou em " << c << endl;
    TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg);
    if (c == 0) {
        TraCIDemo11pMessage* newWsm = createMessage(wsm->getSerial(), myId, wsm->getDemoData(), wsm->getBeginTime());
        t = 0;
        s = 0;
        l.clear();
        sendDown(newWsm);
//        std::cout << "## Reenviado a mensagem" << endl;
    } else {
//        std::cout << "## Mais de uma mensagem foi recebida" << endl;
        simtime_t interArrivalTimeBase = 0.1 / c;
        for(std::vector<simtime_t>::iterator i = l.begin(); i != l.end(); i++) {
            if (interArrivalTimeBase - (*i) > 0) {
                s--;
            } else {
                s++;
            }
        }
//        std::cout << "## O Valor de s ficou em " << s <<endl;
        if (s > 0) {
            TraCIDemo11pMessage* newWsm = createMessage(wsm->getSerial(), myId, wsm->getDemoData(), wsm->getBeginTime());
            t = 0;
            s = 0;
            l.clear();
            sendDown(newWsm);
            std::cout << "## Reenviado a mensagem AQUIII" << endl;
        }
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);
    if (myId % 4 == 0 && simTime() == timeToSendMessage) {
        int rsuId = rsuIds[rand() % 7];
        createAndSendMessage(rsuId, myId, NULL);
        timeToSendMessage = timeToSendMessage + messageSendInterval;
    }
}
