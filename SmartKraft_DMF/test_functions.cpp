#include "test_functions.h"

void TestInterface::begin(CountdownScheduler *sched, MailAgent *mailAgent) {
    scheduler = sched;
    mail = mailAgent;
}

void TestInterface::processSerial() {
    if (!Serial.available()) return;
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "status") {
        ScheduleSnapshot snap = scheduler->snapshot();
        Serial.printf("Aktif: %d, Kalan: %u sn, Alarm index: %u\n",
                      snap.timerActive,
                      snap.remainingSeconds,
                      snap.nextAlarmIndex);
    } else if (command == "start") {
        scheduler->start();
        Serial.println(F("Timer başlatıldı"));
    } else if (command == "reset") {
        scheduler->reset();
        Serial.println(F("Timer sıfırlandı"));
    } else if (command == "stop") {
        scheduler->stop();
        Serial.println(F("Timer durduruldu"));
    } else if (command == "mail") {
        String error;
        ScheduleSnapshot snap = scheduler->snapshot();
        if (mail->sendWarning(snap.nextAlarmIndex, snap, error)) {
            Serial.println(F("Test maili gönderildi"));
        } else {
            Serial.printf("Mail gönderilemedi: %s\n", error.c_str());
        }
    } else {
        Serial.println(F("Komutlar: status, start, reset, stop, mail"));
    }
}
