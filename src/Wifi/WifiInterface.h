/*******************************************************************************
 * Copyright (C) 2018 Gallium Studio LLC (Lawrence Lo). All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Alternatively, this program may be distributed and modified under the
 * terms of Gallium Studio LLC commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for licensees
 * interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Contact information:
 * Website - https://www.galliumstudio.com
 * Source repository - https://github.com/galliumstudio
 * Email - admin@galliumstudio.com
 ******************************************************************************/

#ifndef WIFI_INTERFACE_H
#define WIFI_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "fw_pipe.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

// WIFI_WRITE_CFM - Not used
#define WIFI_INTERFACE_EVT \
    ADD_EVT(WIFI_START_REQ) \
    ADD_EVT(WIFI_START_CFM) \
    ADD_EVT(WIFI_STOP_REQ) \
    ADD_EVT(WIFI_STOP_CFM) \
    ADD_EVT(WIFI_INTERACTIVE_ON_REQ) \
    ADD_EVT(WIFI_INTERACTIVE_ON_CFM) \
    ADD_EVT(WIFI_INTERACTIVE_OFF_REQ) \
    ADD_EVT(WIFI_INTERACTIVE_OFF_CFM) \
    ADD_EVT(WIFI_CONNECT_REQ) \
    ADD_EVT(WIFI_CONNECT_CFM) \
    ADD_EVT(WIFI_DISCONNECT_REQ) \
    ADD_EVT(WIFI_DISCONNECT_CFM) \
    ADD_EVT(WIFI_DISCONNECT_IND) \
    ADD_EVT(WIFI_WRITE_REQ) \
    ADD_EVT(WIFI_WRITE_CFM) \
    ADD_EVT(WIFI_EMPTY_IND) \
    ADD_EVT(WIFI_DATA_IND) \
    ADD_EVT(WIFI_READ_MORE_REQ)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    WIFI_INTERFACE_EVT_START = INTERFACE_EVT_START(WIFI),
    WIFI_INTERFACE_EVT
};

enum {
    WIFI_REASON_UNSPEC = 0,
};

class WifiStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    WifiStartReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(WIFI_START_REQ, to, from, seq) {}
};

class WifiStartCfm : public ErrorEvt {
public:
    WifiStartCfm(Hsmn to, Hsmn from, Sequence seq,
                   Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI_START_CFM, to, from, seq, error, origin, reason) {}
};

class WifiStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    WifiStopReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(WIFI_STOP_REQ, to, from, seq) {}
};

class WifiStopCfm : public ErrorEvt {
public:
    WifiStopCfm(Hsmn to, Hsmn from, Sequence seq,
                   Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI_STOP_CFM, to, from, seq, error, origin, reason) {}
};

class WifiInteractiveOnReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    WifiInteractiveOnReq(Hsmn to, Hsmn from, Sequence seq, Hsmn consoleOutIfHsmn) :
        Evt(WIFI_INTERACTIVE_ON_REQ, to, from, seq),
        m_consoleOutIfHsmn(consoleOutIfHsmn) {}
    Hsmn GetConsoleOutIfHsmn() const { return m_consoleOutIfHsmn; }
private:
    Hsmn m_consoleOutIfHsmn;      // HSMN of the console output interface.
};

class WifiInteractiveOnCfm : public ErrorEvt {
public:
    // In case of error outIfHsmn will be set to HSM_UNDEF.
    WifiInteractiveOnCfm(Hsmn to, Hsmn from, Sequence seq, Hsmn outIfHsmn,
                           Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI_INTERACTIVE_ON_CFM, to, from, seq, error, origin, reason),
        m_outIfHsmn(outIfHsmn) {}
    Hsmn GetOutIfHsmn() const { return m_outIfHsmn; }
private:
    Hsmn m_outIfHsmn;      // HSMN of this object's own output interface.
};

class WifiInteractiveOffReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    WifiInteractiveOffReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(WIFI_INTERACTIVE_OFF_REQ, to, from, seq) {}
};

class WifiInteractiveOffCfm : public ErrorEvt {
public:
    WifiInteractiveOffCfm(Hsmn to, Hsmn from, Sequence seq,
                   Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI_INTERACTIVE_OFF_CFM, to, from, seq, error, origin, reason) {}
};

class WifiConnectReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 5000
    };
    WifiConnectReq(Hsmn to, Hsmn from, Sequence seq, char const *domain, uint16_t port, Fifo *dataOutFifo, Fifo *dataInFifo) :
        Evt(WIFI_CONNECT_REQ, to, from, seq), m_port(port), m_dataOutFifo(dataOutFifo), m_dataInFifo(dataInFifo) {
        STRBUF_COPY(m_domain, domain);
    }
    char const *GetDomain() const { return m_domain; }
    uint16_t GetPort() const { return m_port; }
    Fifo *GetDataOutFifo() const { return m_dataOutFifo; }
    Fifo *GetDataInFifo() const { return m_dataInFifo; }
    enum {
        DOMAIN_LEN = 100,       // Length of domain name or IP address string including null termination.
    };
private:
    char m_domain[DOMAIN_LEN];
    uint16_t m_port;
    Fifo *m_dataOutFifo;
    Fifo *m_dataInFifo;
};

class WifiConnectCfm : public ErrorEvt {
public:
    WifiConnectCfm(Hsmn to, Hsmn from, Sequence seq,
                   Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI_CONNECT_CFM, to, from, seq, error, origin, reason) {}
};

class WifiDisconnectReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 5000
    };
    WifiDisconnectReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(WIFI_DISCONNECT_REQ, to, from, seq) {}
};

class WifiDisconnectCfm : public ErrorEvt {
public:
    WifiDisconnectCfm(Hsmn to, Hsmn from, Sequence seq,
                   Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI_DISCONNECT_CFM, to, from, seq, error, origin, reason) {}
};

class WifiDisconnectInd : public Evt {
public:
    WifiDisconnectInd(Hsmn to, Hsmn from, Sequence seq = 0) :
        Evt(WIFI_DISCONNECT_IND, to, from, seq) {}
};

class WifiWriteReq : public Evt {
public:
    WifiWriteReq(Hsmn to, Hsmn from, Sequence seq = 0) :
        Evt(WIFI_WRITE_REQ, to, from, seq) {
    }
};

class WifiWriteCfm : public ErrorEvt {
public:
    WifiWriteCfm(Hsmn to, Hsmn from, Sequence seq,
                   Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI_WRITE_CFM, to, from, seq, error, origin, reason) {}
};

class WifiEmptyInd : public Evt {
public:
    WifiEmptyInd(Hsmn to, Hsmn from, Sequence seq = 0) :
        Evt(WIFI_EMPTY_IND, to, from, seq) {}
};

class WifiDataInd : public Evt {
public:
    WifiDataInd(Hsmn to, Hsmn from, Sequence seq = 0) :
        Evt(WIFI_DATA_IND, to, from, seq) {}
};

class WifiReadMoreReq : public Evt {
public:
    WifiReadMoreReq(Hsmn to, Hsmn from, Sequence seq = 0) :
        Evt(WIFI_READ_MORE_REQ, to, from, seq) {
    }
};

} // namespace APP

#endif // WIFI_INTERFACE_H
