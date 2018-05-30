/*******************************************************************************
 * Copyright (C) Gallium Studio LLC. All rights reserved.
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

#include "fw_log.h"
#include "fw_assert.h"
#include "app_hsmn.h"
#include "periph.h"
#include "UartActInterface.h"
#include "UartOutInterface.h"
#include "System.h"
#include "SystemInterface.h"
#include "SampleInterface.h"
#include "BtnInterface.h"
#include "LedInterface.h"
#include "bsp.h"

FW_DEFINE_THIS_FILE("System.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "STATE_TIMER",
    "TEST_TIMER",
};

static char const * const internalEvtName[] = {
    "DONE",
    "RESTART",
};

static char const * const interfaceEvtName[] = {
    "SYSTEM_START_REQ",
    "SYSTEM_START_CFM",
    "SYSTEM_STOP_REQ",
    "SYSTEM_STOP_CFM",
};

System::System() :
    Active((QStateHandler)&System::InitialPseudoState, SYSTEM, "SYSTEM",
           timerEvtName, ARRAY_COUNT(timerEvtName),
           internalEvtName, ARRAY_COUNT(internalEvtName),
           interfaceEvtName, ARRAY_COUNT(interfaceEvtName)),
    m_uart1OutFifo(m_uart1OutFifoStor, UART_OUT_FIFO_ORDER),
    m_uart1InFifo(m_uart1InFifoStor, UART_IN_FIFO_ORDER),
    m_stateTimer(this->GetHsm().GetHsmn(), STATE_TIMER),
    m_testTimer(this->GetHsm().GetHsmn(), TEST_TIMER) {}

QState System::InitialPseudoState(System * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&System::Root);
}

QState System::Root(System * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
    case Q_ENTRY_SIG: {
        EVENT(e);
        // Test only
        Periph::SetupNormal();
        Evt *evt;
        evt = new UartActStartReq(UART1_ACT, GET_HSMN(), GEN_SEQ(), &me->m_uart1OutFifo, &me->m_uart1InFifo);
        me->GetHsm().SaveOutSeq(*evt);
        Fw::Post(evt);

        //me->m_testTimer.Start(500, Timer::PERIODIC);
        //evt = new SampleStartReq(SAMPLE, SYSTEM, 0);
        //Fw::Post(evt);
        status = Q_HANDLED();
        break;
    }
    case Q_EXIT_SIG: {
        EVENT(e);
        // Test only.
        me->m_testTimer.Stop();
        status = Q_HANDLED();
        break;
    }
    case TEST_TIMER: {
        //EVENT(e);
        static uint32_t speed = 0;
        static bool up = true;
        static uint32_t stableCounter = 0;
        if (stableCounter) {
            stableCounter--;

        } else {
            if (up) {
                speed += 2;
                if (speed >= 1000) {
                    up = false;
                    stableCounter = 20; // 1s
                }
            } else {
                speed -= 2;
                if (speed <= 0) {
                    up = true;
                    stableCounter = 20; // 1s
                }
            }
        }
        Evt *evt = new LedLevelReq(LED0, GET_HSMN(), GEN_SEQ(), speed);
        Fw::Post(evt);

        // Test only.
        static int testcount = 10000;
        char msg[100];
        snprintf(msg, sizeof(msg), "This is a UART DMA transmission testing number %d (level = %d).", testcount++, speed);
        LOG("Writing %s", msg);
        /*
        bool status = false;
        me->m_uartOutFifo.WriteNoCrit((uint8_t *)msg, strlen(msg), &status);
        Evt *evt = new Evt(UART_OUT_WRITE_REQ, UART2_OUT, GET_HSMN());
        Fw::Post(evt);
        */

        status = Q_HANDLED();
        break;
    }
    case UART_ACT_START_CFM: {
        EVENT(e);
        ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
        if (me->GetHsm().MatchOutSeq(cfm)) {
            if (cfm.GetError() == ERROR_SUCCESS) {
                if(me->GetHsm().IsOutSeqAllCleared()) {
                    LOG("UARTs started successfully");
                    Log::AddInterface(UART1_OUT, &me->m_uart1OutFifo, UART_OUT_WRITE_REQ);

                    //me->m_testTimer.Start(2000, Timer::PERIODIC);

                    Evt *evt = new SampleStartReq(SAMPLE, SYSTEM, 0);
                    Fw::Post(evt);
                    evt = new BtnStartReq(SEL_BTN, GET_HSMN(), GEN_SEQ());
                    Fw::Post(evt);
                    evt = new LedStartReq(LED0, GET_HSMN(), GEN_SEQ());
                    Fw::Post(evt);
                }
            }
        }
        status = Q_HANDLED();
        break;
    }
    case BTN_UP_IND: {
        EVENT(e);
        // Commented for testing.
        //Evt *evt = new LedOffReq(LED0, GET_HSMN(), GEN_SEQ());
        //Fw::Post(evt);
        status = Q_HANDLED();
        break;  
    }
    case BTN_DOWN_IND: {
        EVENT(e);
        // For motor control test.
        static bool once = false;
        if (!once) {
            once = true;
            me->m_testTimer.Start(50, Timer::PERIODIC);
            Evt *evt = new LedLevelReq(LED0, GET_HSMN(), GEN_SEQ(), 0);
            Fw::Post(evt);
            evt = new LedOnReq(LED0, GET_HSMN(), GEN_SEQ());
            Fw::Post(evt);
        }
        status = Q_HANDLED();
        break;  
    }
    case LED_ON_CFM:
    case LED_OFF_CFM: {
        EVENT(e);
        status = Q_HANDLED();
        break;    
    }
    default: {
        status = Q_SUPER(&QHsm::top);
        break;
    }
    }
    return status;
}


} // namespace APP
