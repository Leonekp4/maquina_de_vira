#pragma once
#include "controller.h"
#include "ui_values.h"
#include <cstring>

namespace ihm {
enum class Page { Home, Password, Configuration, Editor, Faults, ResetCount };
enum class Manual { None, MotorForward, MotorReverse, ValveAdvance, ValveReturn };

class Session {
public:
    Page page() const { return page_; }
    Field field() const { return field_; }
    Manual manual() const { return manual_; }
    bool unlocked() const { return unlocked_; }
    void openSettings() {
        release(); unlocked_ = false; page_ = Page::Password;
    }
    bool submitPassword(const char* password) {
        if (page_ != Page::Password || !password || std::strcmp(password, "1234") != 0) return false;
        unlocked_ = true; page_ = Page::Configuration;
        return true;
    }
    void home() {
        release(); unlocked_ = false; page_ = Page::Home;
    }
    void openFaults() {
        release(); unlocked_ = false; page_ = Page::Faults;
    }
    bool openCounter() {
        if (page_ != Page::Home) return false;
        release(); unlocked_ = false; page_ = Page::ResetCount;
        return true;
    }
    template<class Machine>
    bool resetCount(Machine& controller) {
        if (page_ != Page::ResetCount || !controller.resetProduced()) return false;
        home();
        return true;
    }
    template<class Machine>
    bool start(Machine& controller) {
        if (page_ != Page::Home || manual_ != Manual::None) return false;
        if (controller.state() == vira::State::Complete)
            controller.configure(controller.parameters());
        return controller.start();
    }
    bool edit(Field field) {
        if (page_ != Page::Home && page_ != Page::Configuration) return false;
        const bool timing = field == Field::Advance || field == Field::Return;
        if (timing && (page_ != Page::Configuration || !unlocked_)) return false;
        release(); returnPage_ = page_; field_ = field; page_ = Page::Editor;
        return true;
    }
    void closeEditor() {
        if (page_ == Page::Editor) page_ = returnPage_;
    }
    static bool manualAllowed(vira::State state) {
        return state == vira::State::Ready || state == vira::State::Complete;
    }
    bool pressManual(Manual command, vira::State state) {
        if (!manualAllowed(state) || command == Manual::None) return false;
        const bool motor = command == Manual::MotorForward || command == Manual::MotorReverse;
        if (page_ == Page::Home && motor) { manual_ = command; return true; }
        if (page_ == Page::Configuration && unlocked_) { manual_ = command; return true; }
        return false;
    }
    void release() { manual_ = Manual::None; }
    void sync(vira::State state) {
        if (!manualAllowed(state)) release();
    }
private:
    Page page_ = Page::Home, returnPage_ = Page::Home;
    Field field_ = Field::Length;
    Manual manual_ = Manual::None;
    bool unlocked_ = false;
};
inline const char* manualText(Manual command) {
    switch (command) {
    case Manual::None: return "";
    case Manual::MotorForward: return "MOTOR AVANCO";
    case Manual::MotorReverse: return "MOTOR RETORNO";
    case Manual::ValveAdvance: return "VALVULA AVANCO";
    case Manual::ValveReturn: return "VALVULA RETORNO";
    }
    return "";
}
}
