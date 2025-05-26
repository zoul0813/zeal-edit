BIN=edit.bin

ENABLE_CONIO=1
ENABLE_WINDOWS=1

ifdef BREAK
ZOS_CFLAGS += -DBREAK
endif

ifndef ZGDK_PATH
    $(error "Failure: ZGDK_PATH variable not found. It must point to ZGDK path.")
endif

include $(ZGDK_PATH)/base_sdcc.mk


run:
	$(ZEAL_NATIVE_BIN) -H bin -r $(ZEAL_NATIVE_ROM) #-t tf.img -e eeprom.img

native: all run

