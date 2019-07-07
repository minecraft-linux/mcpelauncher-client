#pragma once

#include <minecraft/MinecraftGame.h>

struct OptionsRef {
    Options* ptr;
    std::shared_ptr<Options> sharedPtr;

    OptionsRef(Options* ptr) : ptr(ptr) {}
    OptionsRef(std::shared_ptr<Options> ptr) : ptr(ptr.get()), sharedPtr(std::move(ptr)) {}

    Options* operator->() {
        return ptr;
    }
};

struct MinecraftGameWrapper {

    static MinecraftGameWrapper* create(int argc, char** argv);

    virtual MinecraftGame* getWrapped() = 0;
    virtual void init(AppContext& ctx) = 0;
    virtual void quit(mcpe::string const& cat, mcpe::string const& name) = 0;
    virtual bool wantToQuit() = 0;
    virtual void update() = 0;
    virtual void setRenderingSize(int w, int h) = 0;
    virtual void setUISizeAndScale(int w, int h, float s) = 0;

    virtual OptionsRef getPrimaryUserOptions() = 0;
    virtual void setTextboxText(mcpe::string const& text, int u) = 0;

    virtual void leaveGame() = 0;

};


class MinecraftGameDefaultAppWrapper : public MinecraftGameWrapper {

private:
    App* const app;

public:
    explicit MinecraftGameDefaultAppWrapper(App* app) : app(app) {}

    void init(AppContext& ctx) override {
        app->init(ctx);
    }
    void quit(mcpe::string const& cat, mcpe::string const& name) override {
        app->quit(cat, name);
    }
    bool wantToQuit() override {
        return app->wantToQuit();
    }
    void update() override {
        app->update();
    }
    void setRenderingSize(int w, int h) override {
        app->setRenderingSize(w, h);
    }
    void setUISizeAndScale(int w, int h, float s) override {
        app->setUISizeAndScale(w, h, s);
    }


};

class MinecraftGameDefaultWrapper : public MinecraftGameDefaultAppWrapper {

private:
    MinecraftGame* const game;

    static MinecraftGame* createInstance(int argc, char** argv);

public:
    explicit MinecraftGameDefaultWrapper(MinecraftGame* game) : MinecraftGameDefaultAppWrapper(game), game(game) {}
    MinecraftGameDefaultWrapper(int argc, char** argv) : MinecraftGameDefaultWrapper(createInstance(argc, argv)) {}

    ~MinecraftGameDefaultWrapper() { delete game; }

    MinecraftGame* getWrapped() override { return game; }

    OptionsRef getPrimaryUserOptions() override {
        return game->getPrimaryUserOptions();
    }
    void setTextboxText(mcpe::string const& text, int u) override {
        game->setTextboxText(text, u);
    }

    void leaveGame() override {
        doLeaveGame(game);
    }

    static void doLeaveGame(MinecraftGame* game);

};
