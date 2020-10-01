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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "SensorAccelGyroInterface.h"
#include "SensorMagInterface.h"
#include "SensorHumidTempInterface.h"
#include "SensorPressInterface.h"
#include "SensorInterface.h"
#include "SensorThread.h"
#include "Sensor.h"

FW_DEFINE_THIS_FILE("Sensor.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "SENSOR_TIMER_EVT_START",
    SENSOR_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "SENSOR_INTERNAL_EVT_START",
    SENSOR_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "SENSOR_INTERFACE_EVT_START",
    SENSOR_INTERFACE_EVT
};

// Define I2C and interrupt configurations.
Sensor::Config const Sensor::CONFIG[] = {
    { SENSOR, I2C2, I2C2_EV_IRQn, I2C2_EV_PRIO, I2C2_ER_IRQn, I2C2_ER_PRIO,        // I2C INT
      GPIOB, GPIO_PIN_10, GPIO_PIN_11, GPIO_AF4_I2C2,                              // I2C SCL SDA
      DMA1_Channel4, DMA_REQUEST_3, DMA1_Channel4_IRQn, DMA1_CHANNEL4_PRIO,        // TX DMA
      DMA1_Channel5, DMA_REQUEST_3, DMA1_Channel4_IRQn, DMA1_CHANNEL5_PRIO,        // RX DMA
      ACCEL_GYRO_INT, MAG_DRDY, HUMID_TEMP_DRDY, PRESS_INT
    }
};
I2C_HandleTypeDef Sensor::m_hal;   // Only support single instance.
QXSemaphore Sensor::m_i2cSem;      // Only support single instance.

bool Sensor::I2cWriteInt(uint16_t devAddr, uint16_t memAddr, uint8_t *buf, uint16_t len) {
    if (HAL_I2C_Mem_Write_IT(&m_hal, devAddr, memAddr, I2C_MEMADD_SIZE_8BIT, buf, len) != HAL_OK) {
        return false;
    }
    return m_i2cSem.wait(BSP_MSEC_TO_TICK(1000));
}

bool Sensor::I2cReadInt(uint16_t devAddr, uint16_t memAddr, uint8_t *buf, uint16_t len) {
    // Test only
    //HAL_I2C_Mem_Read(&m_hal, devAddr, memAddr, I2C_MEMADD_SIZE_8BIT, buf, len, 10000);
    //return true;

    // Gallium - test only
    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    if (HAL_I2C_Mem_Read_IT(&m_hal, devAddr, memAddr, I2C_MEMADD_SIZE_8BIT, buf, len) != HAL_OK) {
        return false;
    }
    // Gallium - test only
    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

    // Test only
    //return m_i2cSem.wait(BSP_MSEC_TO_TICK(1000));
    bool result = m_i2cSem.wait(BSP_MSEC_TO_TICK(1000));
    // Gallium - test only
    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    if (!result) {
        return false;
    }
    return true;
}

void Sensor::InitI2c() {
    FW_ASSERT(m_config);
    // GPIO clock enabled in periph.cpp.
    GPIO_InitTypeDef  gpioInit;
    gpioInit.Pin = m_config->sclPin | m_config->sdaPin;

    // Test only
    gpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    gpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(m_config->i2cPort, &gpioInit);
    HAL_GPIO_WritePin(m_config->i2cPort, m_config->sclPin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(m_config->i2cPort, m_config->sdaPin, GPIO_PIN_SET);
    for (uint32_t i=0; i<5; i++) {
    HAL_Delay(1);
    HAL_GPIO_WritePin(m_config->i2cPort, m_config->sdaPin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(m_config->i2cPort, m_config->sclPin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(m_config->i2cPort, m_config->sclPin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(m_config->i2cPort, m_config->sdaPin, GPIO_PIN_SET);
    }

    gpioInit.Mode = GPIO_MODE_AF_OD;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Alternate  = m_config->i2cAf;
    HAL_GPIO_Init(m_config->i2cPort, &gpioInit);

    switch((uint32_t)(m_config->i2c)) {
        case I2C1_BASE: {
            __I2C1_CLK_ENABLE();
            __I2C1_FORCE_RESET();
            __I2C1_RELEASE_RESET();
            break;
        }
        case I2C2_BASE:{
            __I2C2_CLK_ENABLE();
            __I2C2_FORCE_RESET();
            __I2C2_RELEASE_RESET();
            break;
        }
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }

    NVIC_SetPriority(m_config->i2cEvIrq, m_config->i2cEvPrio);
    NVIC_EnableIRQ(m_config->i2cEvIrq);
    NVIC_SetPriority(m_config->i2cErIrq, m_config->i2cErPrio);
    NVIC_EnableIRQ(m_config->i2cErIrq);

    NVIC_SetPriority(m_config->txDmaIrq, m_config->txDmaPrio);
    NVIC_EnableIRQ(m_config->txDmaIrq);
    NVIC_SetPriority(m_config->rxDmaIrq, m_config->rxDmaPrio);
    NVIC_EnableIRQ(m_config->rxDmaIrq);
}

void Sensor::DeInitI2c() {
    switch((uint32_t)(m_hal.Instance)) {
        case I2C1_BASE: __I2C1_FORCE_RESET(); __I2C1_RELEASE_RESET(); __I2C1_CLK_DISABLE(); break;
        case I2C2_BASE: __I2C2_FORCE_RESET(); __I2C2_RELEASE_RESET(); __I2C2_CLK_DISABLE(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }
    HAL_GPIO_DeInit(m_config->i2cPort, m_config->sclPin);
    HAL_GPIO_DeInit(m_config->i2cPort, m_config->sdaPin);
}

bool Sensor::InitHal() {
    memset(&m_hal, 0, sizeof(m_hal));
    memset(&m_txDmaHandle, 0, sizeof(m_txDmaHandle));
    memset(&m_rxDmaHandle, 0, sizeof(m_rxDmaHandle));
#ifdef STM32L475xx
    m_hal.Init.Timing = DISCOVERY_I2Cx_TIMING;
#else
    // Change for other platform (see Examples projects).
    FW_ASSERT(0);
#endif
    m_hal.Init.OwnAddress1    = 0x33;
    m_hal.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    m_hal.Instance            = m_config->i2c;
    if (HAL_I2C_Init(&m_hal) == HAL_OK) {
        return true;
    }
    return false;
}

Sensor::Sensor(XThread &container) :
    Region((QStateHandler)&Sensor::InitialPseudoState, SENSOR, "SENSOR"),
    m_config(&CONFIG[0]), m_client(HSM_UNDEF), m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER), m_container(container),
    m_sensorAccelGyro(m_config->accelGyroIntHsmn, m_hal),
    m_sensorMag(m_config->magDrdyHsmn, m_hal),
    m_sensorHumidTemp(m_config->humidTempDrdyHsmn, m_hal),
    m_sensorPress(m_config->pressIntHsmn, m_hal) {
    SET_EVT_NAME(SENSOR);
    m_i2cSem.init(0,1);
}

QState Sensor::InitialPseudoState(Sensor * const me, QEvt const * const e) {
    (void)e;
    // We need to use m_container rather than GetHsm().GetContainer() since we need the derived type XThread rather than QActive.
    me->m_sensorAccelGyro.Init(&me->m_container);
    me->m_sensorHumidTemp.Init(&me->m_container);
    me->m_sensorMag.Init(&me->m_container);
    me->m_sensorPress.Init(&me->m_container);
    return Q_TRAN(&Sensor::Root);
}

QState Sensor::Root(Sensor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            //EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Sensor::Stopped);
        }
        case SENSOR_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new SensorStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
            Fw::Post(evt);
            return Q_HANDLED();
        }
        case SENSOR_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->GetHsm().SaveInSeq(req);
            return Q_TRAN(&Sensor::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Sensor::Stopped(Sensor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            //EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SENSOR_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new SensorStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            return Q_HANDLED();
        }
        case SENSOR_START_REQ: {
            EVENT(e);
            SensorStartReq const &req = static_cast<SensorStartReq const &>(*e);
            me->InitI2c();
            if (me->InitHal()) {
                me->m_client = req.GetFrom();
                me->GetHsm().SaveInSeq(req);
                Evt *evt = new Evt(START, GET_HSMN());
                me->PostSync(evt);
            } else {
                ERROR("InitHal() failed");
                Evt *evt = new SensorStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_HAL, GET_HSMN());
                Fw::Post(evt);
            }
            return Q_HANDLED();
        }
        case START: {
            EVENT(e);
            return Q_TRAN(&Sensor::Starting);
        }
    }
    return Q_SUPER(&Sensor::Root);
}

QState Sensor::Starting(Sensor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SensorStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > SensorAccelGyroStartReq::TIMEOUT_MS);
            FW_ASSERT(timeout > SensorMagStartReq::TIMEOUT_MS);
            FW_ASSERT(timeout > SensorHumidTempStartReq::TIMEOUT_MS);
            FW_ASSERT(timeout > SensorPressStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->GetHsm().ResetOutSeq();
            Evt *evt = new SensorAccelGyroStartReq(SENSOR_ACCEL_GYRO, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            evt = new SensorMagStartReq(SENSOR_MAG, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            evt = new SensorHumidTempStartReq(SENSOR_HUMID_TEMP, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            evt = new SensorPressStartReq(SENSOR_PRESS, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->GetHsm().ClearInSeq();
            return Q_HANDLED();
        }
        case SENSOR_ACCEL_GYRO_START_CFM:
        case SENSOR_MAG_START_CFM:
        case SENSOR_HUMID_TEMP_START_CFM:
        case SENSOR_PRESS_START_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->GetHsm().HandleCfmRsp(cfm, allReceived)) {
                Evt *evt = new Failed(GET_HSMN(), cfm.GetError(), cfm.GetOrigin(), cfm.GetReason());
                me->PostSync(evt);
            } else if (allReceived) {
                Evt *evt = new Evt(DONE, GET_HSMN());
                me->PostSync(evt);
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            Evt *evt;
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                evt = new SensorStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(),
                                            failed.GetError(), failed.GetOrigin(), failed.GetReason());
            } else {
                evt = new SensorStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_TIMEOUT, GET_HSMN());
            }
            Fw::Post(evt);
            return Q_TRAN(&Sensor::Stopping);
        }
        case DONE: {
            EVENT(e);
            Evt *evt = new SensorStartCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            return Q_TRAN(&Sensor::Started);
        }
    }
    return Q_SUPER(&Sensor::Root);
}

QState Sensor::Stopping(Sensor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SensorStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > SensorAccelGyroStopReq::TIMEOUT_MS);
            FW_ASSERT(timeout > SensorMagStopReq::TIMEOUT_MS);
            FW_ASSERT(timeout > SensorHumidTempStopReq::TIMEOUT_MS);
            FW_ASSERT(timeout > SensorPressStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->GetHsm().ResetOutSeq();
            Evt *evt = new SensorAccelGyroStopReq(SENSOR_ACCEL_GYRO, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            evt = new SensorMagStopReq(SENSOR_MAG, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            evt = new SensorHumidTempStopReq(SENSOR_HUMID_TEMP, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            evt = new SensorPressStopReq(SENSOR_PRESS, GET_HSMN(), GEN_SEQ());
            me->GetHsm().SaveOutSeq(*evt);
            Fw::Post(evt);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->GetHsm().ClearInSeq();
            me->GetHsm().Recall();
            return Q_HANDLED();
        }
        case SENSOR_STOP_REQ: {
            EVENT(e);
            me->GetHsm().Defer(e);
            return Q_HANDLED();
        }
        case SENSOR_ACCEL_GYRO_STOP_CFM:
        case SENSOR_MAG_STOP_CFM:
        case SENSOR_HUMID_TEMP_STOP_CFM:
        case SENSOR_PRESS_STOP_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->GetHsm().HandleCfmRsp(cfm, allReceived)) {
                Evt *evt = new Failed(GET_HSMN(), cfm.GetError(), cfm.GetOrigin(), cfm.GetReason());
                me->PostSync(evt);
            } else if (allReceived) {
                Evt *evt = new Evt(DONE, GET_HSMN());
                me->PostSync(evt);
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            Evt *evt = new SensorStopCfm(me->GetHsm().GetInHsmn(), GET_HSMN(), me->GetHsm().GetInSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            HAL_I2C_DeInit(&me->m_hal);
            me->DeInitI2c();
            return Q_TRAN(&Sensor::Stopped);
        }
    }
    return Q_SUPER(&Sensor::Root);
}

QState Sensor::Started(Sensor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Sensor::Root);
}

/*
QState Sensor::MyState(Sensor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Sensor::SubState);
        }
    }
    return Q_SUPER(&Sensor::SuperState);
}
*/

} // namespace APP
