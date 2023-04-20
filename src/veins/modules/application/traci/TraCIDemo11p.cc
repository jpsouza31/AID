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

using namespace veins;

Define_Module(veins::TraCIDemo11p);

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);
    if (myId % 4 == 0 && simTime() > 65 && simTime() == timeToSendMessage) {
        messageGeneratedCount++;
        int rsuId = rsuIds[rand() % 4];
        createAndSendMessage(rsuId, myId, messageGeneratedCount, rsuId, NULL);
        timeToSendMessage = timeToSendMessage + messageSendInterval;
    }
}

void TraCIDemo11p::createAndSendMessage (int to, int senderId,  int messageId, int finalAddress, simtime_t beginTime = NULL) {
    std::string dado = "Mensagem para o node " + std::to_string(to) + " enviado do " + std::to_string(myId);
    TraCIDemo11pMessage* wsm = createMessage(to, senderId, dado, messageId, finalAddress, beginTime);
    sendDown(wsm);
}

TraCIDemo11pMessage* TraCIDemo11p::createMessage (int to, int senderId, std::string dado,  int messageId, int finalAddress, simtime_t beginTime = NULL) {
    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
    populateWSM(wsm, -1);
    wsm->setFinalAddress(finalAddress);
    wsm->setSenderAddress(senderId);
//    wsm->setByteLength(100000);
    wsm->setDemoData(dado.c_str());
    wsm->setMessageId(messageId);
    if (beginTime == NULL)
        wsm->setBeginTime(simTime());
    else
        wsm->setBeginTime(beginTime);
    return wsm;
}

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
        messageSendInterval = 15;
        timeToSendMessage = simTime() + messageSendInterval;
        rsuIds[0] = 13;
        rsuIds[1] = 18;
        rsuIds[2] = 23;
        rsuIds[3] = 28;
        messageGeneratedCount = 0;
        replicatedMessagesCount = 0;
        receivedMessagesCount = 0;
        queue = 0;
        queueSize = 20000;
        nNos = "200";
        qSize = "20000";
//        messageGeneratedCountVec.setName("messageGeneratedCount");
        runNumber = std::to_string(getEnvir()->getConfigEx()->getActiveRunNumber());
        stackOverflowNumber = 0;
    }
}

void TraCIDemo11p::finish()
{
    DemoBaseApplLayer::finish();
//    cancelEvent(newWsm);

//    recordScalar("geradas", messageGeneratedCount);
//    recordScalar("replicadas", replicatedMessagesCount);
//    getSimulation()->getS

//            std::cout << "AQUII " << getEnvir()->getConfigEx()->getActiveRunNumber() << endl;
//    messageGeneratedCountVec.record(messageGeneratedCount);


    std::ofstream out;
    out.open("/home/joao/Documentos/TCC/aid/resultados/" + runNumber + "/geradas" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << messageGeneratedCount << std::endl;
    out.close();
    out.open("/home/joao/Documentos/TCC/aid/resultados/" + runNumber + "/replicadas" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << replicatedMessagesCount << std::endl;
    out.close();
    out.open("/home/joao/Documentos/TCC/aid/resultados/" + runNumber + "/overflow" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << stackOverflowNumber << std::endl;
    out.close();
    out.open("/home/joao/Documentos/TCC/aid/resultados/" + runNumber + "/recebidas" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << receivedMessagesCount << std::endl;
    out.close();
//    std::cout << "FOI GERADA ESSA QUANTIDADE DE MENSAGENS DO NODE " << myId << ": " << messageGeneratedCount << endl;
//    std::cout << "FOI GERADA ESSA QUANTIDADE DE MENSAGENS REPLICADAS DO NODE " << myId << ": " << replicatedMessagesCount << endl;
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
    if (wsm->getBeginTime() + 0.2 > simTime()) {
        receivedMessagesCount++;
        if (getCurrentQueueSize() + 1 < getQueueSize()) {
            addTaskSizeToQueue(1);
//            if (wsm->getBeginTime() + 0.5 > simTime()) {
                if (wsm->getSenderAddress() != myId) {
                    std::map<int, int> cControl = cControlMap[wsm->getSenderAddress()];
                    std::map<int, int> sControl = sControlMap[wsm->getSenderAddress()];
                    std::map<int, simtime_t> tControl = tControlMap[wsm->getSenderAddress()];
                    std::map<int, std::vector<simtime_t>> lControl = lControlMap[wsm->getSenderAddress()];
                    int c = cControl[wsm->getMessageId()];
                    int s = sControl[wsm->getMessageId()];
                    simtime_t t = tControl[wsm->getMessageId()];
                    std::vector<simtime_t> l = lControl[wsm->getMessageId()];

                    if (t == NULL) t = 0;

                    if (t == 0) {
                        c = 1;
                        s = 0;
                        t = simTime();
                        newWsm = createMessage(wsm->getSerial(), wsm->getSenderAddress(), wsm->getDemoData(), wsm->getMessageId(), wsm->getFinalAddress(), wsm->getBeginTime());
                        //        sendDown(newWsm);
                        delay = uniform(0, par("randomDelayTimeMax").doubleValue());
                        scheduleAt(simTime() + delay, newWsm);
                    } else {
                        c++;
                        simtime_t t1 = simTime();
                        l.push_back(t1 - t);
                        t = t1;
                    }

                    cControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = c;
                    sControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = s;
                    tControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = t;
                    lControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = l;
                }
//            }
        } else stackOverflowNumber++;
    } else {
//        std::cout << "AQUI"<< endl;
    }
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg);

    std::map<int, int> cControl = cControlMap[wsm->getSenderAddress()];
    std::map<int, int> sControl = sControlMap[wsm->getSenderAddress()];
    std::map<int, simtime_t> tControl = tControlMap[wsm->getSenderAddress()];
    std::map<int, std::vector<simtime_t>> lControl = lControlMap[wsm->getSenderAddress()];

    int c = cControl[wsm->getMessageId()];
    int s = sControl[wsm->getMessageId()];
    simtime_t t = tControl[wsm->getMessageId()];
    std::vector<simtime_t> l = lControl[wsm->getMessageId()];

    if (c == 1) {
        TraCIDemo11pMessage* newWsm = createMessage(wsm->getSerial(), wsm->getSenderAddress(), wsm->getDemoData(), wsm->getMessageId(), wsm->getFinalAddress(), wsm->getBeginTime());
        t = 0;
        l.clear();
        sendDown(newWsm);
        replicatedMessagesCount++;
    } else {
        simtime_t interArrivalTimeBase = delay / c;
        for(std::vector<simtime_t>::iterator i = l.begin(); i != l.end(); i++) {
//            std::cout << "AQUII " << *i << endl;
           if (interArrivalTimeBase - (*i) > 0) {
               s--;
           } else {
               s++;
           }
        }
        if (s > 0) {
            TraCIDemo11pMessage* newWsm = createMessage(wsm->getSerial(), wsm->getSenderAddress(), wsm->getDemoData(), wsm->getMessageId(), wsm->getFinalAddress(), wsm->getBeginTime());
            t = 0;
            l.clear();
            sendDown(newWsm);
            replicatedMessagesCount++;
        } else {
//            std::cout << myId << " - DECIDI MÂO ENVIAR " << endl;
        }
    }

    removeTaskSizeToQueue(1);
    cControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = c;
    sControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = s;
    tControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = t;
    lControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = l;
}

int TraCIDemo11p::getQueueSize() {
    return queueSize;
}

void TraCIDemo11p::addTaskSizeToQueue(int value) {
    queue = queue + value;
}

void TraCIDemo11p::removeTaskSizeToQueue(int value) {
    queue = queue - value;
}

int TraCIDemo11p::getCurrentQueueSize() {
    return queue;
}
