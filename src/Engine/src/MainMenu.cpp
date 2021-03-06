#include "MainMenu.hpp"
#include "Timing.hpp"
#include "Globals.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "TournamentFactory.hpp"
#include "OptionsMenu.hpp"
#include "Protocol.hpp"
#include "Scope.hpp"

#include <cstdlib>
#include <algorithm>

const char *TitleMusic = "norway";

MainMenu::MainMenu(Resources& resources, Subsystem& subsystem, Configuration& config)
    : Gui(resources, subsystem, resources.get_font("normal")),
      OptionsMenu(*this, resources, subsystem, config, false),
      resources(resources), subsystem(subsystem), config(config),
      x(0), y(0), bgox(0), bgoy(0), goat(0), title(0), gw(0), gh(0),
      shown(MenuButtonStateNone), lan_broadcaster(0), master_query(0),
      main_window(0), mw_w(0), mw_h(0),
      menu_construction(resources.get_sound("menu_construction"))
{
    goat = resources.get_icon("title_goat");
    title = resources.get_icon("title_text");

    gw = goat->get_tile()->get_tilegraphic()->get_width();
    gh = goat->get_tile()->get_tilegraphic()->get_height();

    brick = resources.get_icon("title_brick");
    bw = brick->get_tile()->get_tilegraphic()->get_width();
    bh = brick->get_tile()->get_tilegraphic()->get_height();

    get_now(startup);
    last = startup;

    subsystem.play_music(resources.get_music(TitleMusic));

    memset(rb, 0, sizeof rb);
}

MainMenu::~MainMenu() {
    if (lan_broadcaster) {
        delete lan_broadcaster;
    }
}

void MainMenu::idle() throw (Exception) {
    static char buffer[64];
    get_now(now);

    /* first delay, then show main menu */
    if (shown != MenuButtonStateDone) {
        ms_t diff = diff_ms(startup, now);
        switch (shown) {
            case MenuButtonStateNone:
            {
                if (diff > 1300) {
                    startup = now;
                    shown = MenuButtonStateCreating;

                    int vw = subsystem.get_view_width();
                    int vh = subsystem.get_view_height();
                    mw_w = 600;
                    mw_h = 300;
                    main_window = push_window(vw / 2 - mw_w / 2, vh / 2 - mw_h / 2, mw_w, mw_h, "");
                    main_window->set_invisible(true);
                    set_mouse_xy(subsystem.get_view_width() / 2, subsystem.get_view_height() / 2);
                }
                break;
            }

            case MenuButtonStateCreating:
            {
                if (diff > 50) {
                    startup = now;

                    int bw = 160;
                    int bh = 35;
                    const int yofs = 30;

                    if (!rb[0]) {
                        rb[0] = create_rounded_button(main_window, 20, 30 + yofs, bw, bh, "Play", static_play_click, this);
                        rb[0]->set_follow_alpha(false);
                        subsystem.play_sound(menu_construction, 1);
                    } else if (!rb[1]) {
                        rb[1] = create_rounded_button(main_window, 17, 90 + yofs, bw, bh, "Create Local LAN Server", static_create_server_click, this);
                        rb[1]->set_follow_alpha(false);
                        subsystem.play_sound(menu_construction, 1);
                    } else if (!rb[2]) {
                        rb[2] = create_rounded_button(main_window, 20, 150 + yofs, bw, bh, "List Loaded Packages", static_list_packages_click, this);
                        rb[2]->set_follow_alpha(false);
                        subsystem.play_sound(menu_construction, 1);
                    } else if (!rb[3]) {
                        rb[3] = create_rounded_button(main_window, 420, 30 + yofs, bw, bh, "Options And Settings", static_options_click, this);
                        rb[3]->set_follow_alpha(false);
                        subsystem.play_sound(menu_construction, 1);
                    } else if (!rb[4]) {
                        rb[4] = create_rounded_button(main_window, 423, 90 + yofs, bw, bh, "Credits", static_credits_click, this);
                        rb[4]->set_follow_alpha(false);
                        subsystem.play_sound(menu_construction, 1);
                    } else if (!rb[5]) {
                        rb[5] = create_rounded_button(main_window, 420, 150 + yofs, bw, bh, "Quit Game", static_quit_click, this);
                        rb[5]->set_follow_alpha(false);
                        subsystem.play_sound(menu_construction, 1);
                        shown = MenuButtonStateFinalize;
                    }
                }
                break;
            }

            case MenuButtonStateFinalize:
            {
                if (diff > 100) {
                    startup = now;
                    shown = MenuButtonStateDone;

                    Font *f = get_font();
                    int th = f->get_font_height();
                    std::string version("Current Version: ");
                    version += GameVersion;
                    int tw = f->get_text_width(version);
                    create_label(main_window, mw_w / 2 - tw / 2, mw_h - th - 21, version)->set_follow_alpha(false);

                    if (ProductIsBeta) {
                        Icon *beta = resources.get_icon("beta");
                        create_picture(main_window, mw_w / 2 + tw / 2, mw_h - 32 - Spc - 3, beta->get_tile()->get_tilegraphic())->set_follow_alpha(false);
                    }
                }
                break;
            }

            case MenuButtonStateDone:
                break;
        }
    }

    int vw = subsystem.get_view_width();
    int vh = subsystem.get_view_height();

    /* retrieving LAN server infos */
    if (lan_broadcaster) {
        /* refresh */
        ms_t diff = diff_ms(last_lan_info, now);
        if (diff > 5000) {
            try {
                Scope<Mutex> lock(lan_broadcaster->get_mutex());
                lan_broadcaster->refresh();
            } catch (const Exception&) {
                /* chomp */
            }
            last_lan_info = now;
        }
        /* update list */
        int top_index = play_lan_list->get_top_index();
        play_lan_list->clear();
        Scope<Mutex> lock(lan_broadcaster->get_mutex());
        const Hosts& hosts = lan_broadcaster->get_hosts();
        for (Hosts::const_iterator it = hosts.begin();
            it != hosts.end(); it++)
        {
            const GameserverInformation *info = *it;
            GuiListboxEntry *entry = play_lan_list->add_entry(info->server_name);
            entry->add_column((info->secured ? "\x7f" : ""), 8);
            sprintf(buffer, "%d/%d", info->cur_clients, info->max_clients);
            entry->add_column(buffer, 50);
            sprintf(buffer, "%d", static_cast<int>(info->ping_time));
            entry->add_column(buffer, 40);
            entry->set_ptr_tag(info);
        }
        play_lan_list->set_top_index(top_index);
    }

    /* retrieving WAN server infos */
    if (master_query) {
        /* update list */
        int top_index = play_wan_list->get_top_index();
        play_wan_list->clear();
        Scope<Mutex> lock(master_query->get_mutex());
        const Hosts& hosts = master_query->get_hosts();
        for (Hosts::const_iterator it = hosts.begin();
            it != hosts.end(); it++)
        {
            const MasterQueryClient *info = static_cast<MasterQueryClient *>(*it);
            if (info->received) {
                GuiListboxEntry *entry = play_wan_list->add_entry(info->server_name);
                entry->add_column((info->secured ? "\x7f" : ""), 8);
                sprintf(buffer, "%d/%d", info->cur_clients, info->max_clients);
                entry->add_column(buffer, 50);
                sprintf(buffer, "%d", static_cast<int>(info->ping_time));
                entry->add_column(buffer, 40);
                entry->set_ptr_tag(info);
                if (info == wan_list_selected_entry) {
                    play_wan_list->set_selected_index(play_wan_list->get_entry_count() - 1);
                }
            }
        }
        play_wan_list->set_top_index(top_index);
    }

    /* draw background */
    ms_t diff = diff_ms(last, now);
    if (diff > 25) {
        last = now;
        bgox--;
        bgoy--;
        bgox %= bw;
        bgoy %= bh;
    }
    y = bgoy;
    subsystem.set_color(1.0f, 1.0f, 1.0f, 0.35f);
    while (y < vh) {
        x = bgox;
        while (x < vw) {
            subsystem.draw_icon(brick, x, y);
            x += bw;
        }
        y += bh;
    }
    subsystem.reset_color();

    /* draw goat */
    subsystem.set_color(1.0f, 1.0f, 1.0f, 1);
    x = vw / 2 - gw / 2;
    y = vh / 2 - gh / 2;
    subsystem.draw_icon(title, x, y);
    subsystem.draw_icon(goat, x, y);
    subsystem.reset_color();
}

void MainMenu::on_input_event(const InputData& input) { }

void MainMenu::on_leave() { }

/* ************************************************************************** */
/* Play                                                                       */
/* ************************************************************************** */
void MainMenu::static_play_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->play_click();
}

void MainMenu::play_click() {
    if (!create_server_locators()) {
        return;
    }

    int vw = subsystem.get_view_width();
    int vh = subsystem.get_view_height();
    int ww = 400;
    int wh = 300;

    GuiWindow *window = push_window(vw / 2 - ww / 2, vh / 2 - wh / 2, ww, wh, "Play");
    window->set_cancelable(true);
    window->set_cancel_callback(static_cancel_click, this);

    int wcw = window->get_client_width();
    int wch = window->get_client_height();

    int bw = 55;
    int bh = 18;
    //int bw2 = 170;
    create_button(window, wcw / 2 - bw / 2, window->get_client_height() - bh - Spc, bw, bh, "Close", static_play_close, this);

    //create_button(window, wcw - bw - Spc, window->get_client_height() - bh - Spc, bw, bh, "Close", static_play_close, this);
    //create_button(window, Spc, window->get_client_height() - bh - Spc, bw2, bh, "Enter Address And Connect", static_play_manual, this);

    GuiTab *tab = create_tab(window, Spc, Spc, wcw - (2 * Spc), wch - bh - (3 * Spc));

    bw = 60;

    GuiListboxEntry *title_bar;

    /* internet */
    GuiFrame *frwan = tab->create_tab("Internet");
    create_label(frwan, 0, 0, "Internet servers:");
    play_wan_list = create_listbox(frwan, 0, 15, frwan->get_width(), frwan->get_height() + 2 - (15 + bh + Spc), "Server Name", static_on_wan_entry_click, this);
    play_wan_list->show_title_bar(true);
    play_wan_list->set_on_title_clicked(static_play_wan_sort_click, this);

    create_button(frwan, frwan->get_width() - bw, frwan->get_height() - bh, bw, bh, "Connect", static_play_connect_wan_click, this);
    create_button(frwan, 0, frwan->get_height() - bh, bw, bh, "Refresh", static_play_refresh_wan_click, this);

    title_bar = play_wan_list->get_title_bar();
    title_bar->add_column("\x7f", 8);
    title_bar->add_column("Players", 50);
    title_bar->add_column("Ping", 40);

    /* LAN */
    GuiFrame *frlan = tab->create_tab("LAN");
    create_label(frlan, 0, 0, "LAN servers:");
    play_lan_list = create_listbox(frlan, 0, 15, frlan->get_width(), frlan->get_height() + 2 - (15 + bh + Spc), "Server Name", static_on_lan_entry_click, this);
    play_lan_list->show_title_bar(true);
    play_lan_list->set_on_title_clicked(static_play_lan_sort_click, this);
    create_button(frlan, frlan->get_width() - bw, frlan->get_height() - bh, bw, bh, "Connect", static_play_connect_lan_click, this);
    create_button(frlan, 0, frlan->get_height() - bh, bw, bh, "Refresh", static_play_refresh_lan_click, this);

    title_bar = play_lan_list->get_title_bar();
    title_bar->add_column("\x7f", 8);
    title_bar->add_column("Players", 50);
    title_bar->add_column("Ping", 40);

    /* install sort handlers */
    wan_list_selected_entry = 0;
    lan_list_selected_entry = 0;
}

void MainMenu::static_on_wan_entry_click(GuiListbox *sender, void *data, int index) {
    (reinterpret_cast<MainMenu *>(data))->on_wan_entry_click(index);
}

void MainMenu::on_wan_entry_click(int index) {
    wan_list_selected_entry = play_wan_list->get_entry(index)->get_ptr_tag();
}

void MainMenu::static_on_lan_entry_click(GuiListbox *sender, void *data, int index) {
    (reinterpret_cast<MainMenu *>(data))->on_lan_entry_click(index);
}

void MainMenu::on_lan_entry_click(int index) {
    lan_list_selected_entry = play_lan_list->get_entry(index)->get_ptr_tag();
}

void MainMenu::static_cancel_click(GuiObject *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->play_close();
}

void MainMenu::static_play_close(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->play_close();
}

void MainMenu::play_close() {
    delete lan_broadcaster;
    delete master_query;
    lan_broadcaster = 0;
    master_query = 0;
    pop_window();
}

void MainMenu::static_play_wan_sort_click(GuiListbox *sender, void *data, int index) {
    (reinterpret_cast<MainMenu *>(data))->play_wan_sort_click(index);
}

void MainMenu::play_wan_sort_click(int index) {
    if (master_query) {
        master_query->sort(index);
    }
}

void MainMenu::static_play_lan_sort_click(GuiListbox *sender, void *data, int index) {
    (reinterpret_cast<MainMenu *>(data))->play_lan_sort_click(index);
}

void MainMenu::play_lan_sort_click(int index) {
    if (lan_broadcaster) {
        lan_broadcaster->sort(index);
    }
}

void MainMenu::static_play_connect_lan_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->play_connect_lan_click();
}

void MainMenu::play_connect_lan_click() {
    int index = play_lan_list->get_selected_index();
    const Hosts& hosts = lan_broadcaster->get_hosts();
    int sz = static_cast<int>(hosts.size());

    if (index >= 0 && index < sz) {
        const GameserverInformation *info = hosts[index];
        hostaddr_t host = info->host;
        hostport_t port = info->port;
        std::string pwd;
        if (info->secured) {
            if (show_inputbox("Enter password:", pwd, true) != MessageBoxResponseYes) {
                return;
            }
        }
        if (info->protocol_version != ProtocolVersion) {
            show_messagebox(Gui::MessageBoxIconError, "Version Mismatch", "Both protocols differ, you cannot connect to this server.");
        } else {
            lan_broadcaster->stop();
            master_query->stop();
            try {
                ScopeMusicStopper stop_music(subsystem, resources.get_music(TitleMusic));
                Client client(resources, subsystem, host, port, config, pwd);
                client.link_mouse(*this);
                client.run();
            } catch (const Exception& e) {
                show_messagebox(Gui::MessageBoxIconError, "Error", e.what());
            }
            lan_broadcaster->start();
            master_query->start();
        }
    } else {
        show_messagebox(Gui::MessageBoxIconExclamation, "Server", "Please select a server first.");
    }
}

void MainMenu::static_play_refresh_lan_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->play_refresh_lan_click();
}

void MainMenu::play_refresh_lan_click() {
    Scope<Mutex> lock(lan_broadcaster->get_mutex());
    lan_list_selected_entry = 0;
    play_lan_list->set_selected_index(-1);
    lan_broadcaster->clear();
    lan_broadcaster->refresh();
    get_now(last_lan_info);
}

void MainMenu::static_play_refresh_wan_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->play_refresh_wan_click();
}

void MainMenu::play_refresh_wan_click() {
    Scope<Mutex> lock(master_query->get_mutex());
    wan_list_selected_entry = 0;
    play_wan_list->set_selected_index(-1);
    master_query->refresh();
}

void MainMenu::static_play_connect_wan_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->play_connect_wan_click();
}

void MainMenu::play_connect_wan_click() {
    int index = play_wan_list->get_selected_index();
    const Hosts& hosts = master_query->get_hosts();
    int sz = static_cast<int>(hosts.size());

    if (index >= 0 && index < sz) {
        MasterQueryClient *info = static_cast<MasterQueryClient *>(hosts[index]);
        std::string pwd;
        if (info->secured) {
            if (show_inputbox("Enter password:", pwd, true) != MessageBoxResponseYes) {
                return;
            }
        }
        hostaddr_t host = info->host;
        hostport_t port = info->port;
        if (info->protocol_version != ProtocolVersion) {
            show_messagebox(Gui::MessageBoxIconError, "Version Mismatch", "Both protocols differ, you cannot connect to this server.");
        } else {
            lan_broadcaster->stop();
            master_query->stop();
            try {
                ScopeMusicStopper stop_music(subsystem, resources.get_music(TitleMusic));
                Client client(resources, subsystem, host, port, config, pwd);
                client.link_mouse(*this);
                client.run();
            } catch (const Exception& e) {
                show_messagebox(Gui::MessageBoxIconError, "Error", e.what());
            }
            lan_broadcaster->start();
            master_query->start();
        }
    } else {
        show_messagebox(Gui::MessageBoxIconExclamation, "Server", "Please select a server first.");
    }
}

void MainMenu::static_play_manual(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->play_manual();
}

void MainMenu::play_manual() {

}

/* ************************************************************************** */
/* Create server                                                              */
/* ************************************************************************** */
void MainMenu::static_create_server_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->create_server_click();
}

void MainMenu::create_server_click() {
    Font *f = get_font();
    int vw = subsystem.get_view_width();
    int vh = subsystem.get_view_height();
    int ww = 337;
    int wh = 326;
    int bw = 140;
    GuiWindow *window = push_window(vw / 2 - ww / 2, vh / 2 - wh / 2, ww, wh, "Create Local LAN Server");
    window->set_cancelable(true);

    /* server settings */
    create_label(window, 15, 15, "server name:");
    cs_server_name = create_textbox(window, 100, 15, 221, config.get_string("server_name"));
    cs_server_name->set_focus();

    create_label(window, 15, 35, "port:");
    cs_server_port = create_textbox(window, 100, 35, 55, config.get_string("server_port"));

    create_box(window, 15, 55, ww - 30, 1);

    /* game modes */
    int game_mode = config.get_int("game_mode");
    create_label(window, 15, 60, "game mode:");
    cs_dm = create_checkbox(window, 100, 61, "death match", game_mode == GamePlayTypeDM, static_game_mode_click, this);
    cs_tdm = create_checkbox(window, 100, 74, "team death match", game_mode == GamePlayTypeTDM, static_game_mode_click, this);
    cs_ctf = create_checkbox(window, 100, 87, "capture the flag", game_mode == GamePlayTypeCTF, static_game_mode_click, this);

    cs_sr = create_checkbox(window, 220, 61, "speed race", game_mode == GamePlayTypeSR, static_game_mode_click, this);
    cs_ctc = create_checkbox(window, 220, 74, "catch the coin", game_mode == GamePlayTypeCTC, static_game_mode_click, this);
    cs_goh = create_checkbox(window, 220, 87, "goat of the hill", game_mode == GamePlayTypeGOH, static_game_mode_click, this);

    cs_current_mode = cs_dm;
    if (game_mode == GamePlayTypeTDM) {
        cs_current_mode = cs_tdm;
    } else if (game_mode == GamePlayTypeCTF) {
        cs_current_mode = cs_ctf;
    } else if (game_mode == GamePlayTypeSR) {
        cs_current_mode = cs_sr;
    } else if (game_mode == GamePlayTypeCTC) {
        cs_current_mode = cs_ctc;
    } else if (game_mode == GamePlayTypeGOH) {
        cs_current_mode = cs_goh;
    }

    cs_dm->set_style(GuiCheckbox::CheckBoxStyleCircle);
    cs_dm->set_tag(GamePlayTypeDM);

    cs_tdm->set_style(GuiCheckbox::CheckBoxStyleCircle);
    cs_tdm->set_tag(GamePlayTypeTDM);

    cs_ctf->set_style(GuiCheckbox::CheckBoxStyleCircle);
    cs_ctf->set_tag(GamePlayTypeCTF);

    cs_sr->set_style(GuiCheckbox::CheckBoxStyleCircle);
    cs_sr->set_tag(GamePlayTypeSR);

    cs_ctc->set_style(GuiCheckbox::CheckBoxStyleCircle);
    cs_ctc->set_tag(GamePlayTypeCTC);

    cs_goh->set_style(GuiCheckbox::CheckBoxStyleCircle);
    cs_goh->set_tag(GamePlayTypeGOH);


    create_box(window, 15, 104, ww - 30, 1);

    /* max players and duration */
    create_label(window, 15, 111, "max. players:");
    cs_max_players = create_textbox(window, 100, 111, 55, config.get_string("max_players"));

    create_label(window, 15, 131, "warm up:");
    cs_warmup = create_textbox(window, 100, 131, 55, config.get_string("warmup"));
    create_label(window, 160, 131, "(in seconds)");

    create_label(window, 15, 151, "duration:");
    cs_duration = create_textbox(window, 100, 151, 55, config.get_string("duration"));
    create_label(window, 160, 151, "(in minutes)");

    create_box(window, 15, 171, ww - 30, 1);

    /* map selector */
    Icon *no_map = resources.get_icon("map_preview");
    cs_map_preview = create_picture(window, ww - 64 - 15, 193, no_map->get_tile()->get_tilegraphic());
    cs_map_name = create_label(window, ww - 64 - 15, 193 + 64 + 3, "");

    create_label(window, 15, 175, "select map:");
    cs_maps = create_listbox(window, 15, 193, 235, 80, "Map Name", static_map_selected, this);

    fill_map_listbox(static_cast<GamePlayType>(game_mode));

    /* buttons */
    std::string text("Cancel");
    bw = f->get_text_width(text) + 28;
    create_button(window, ww - bw - 17, wh - 43, bw, 18, text, static_close_window_click, this);


    text = "Start Server";
    bw = f->get_text_width(text) + 28;
    int last_bw = bw;
    create_button(window, 15, wh - 43, bw, 18, text, static_start_server_click, this);

    text = "Close";
    bw = f->get_text_width(text) + 28;
    create_button(window, last_bw + 20, wh - 43, bw, 18, text, static_close_start_server_click, this);
}

bool MainMenu::MapEntryCompare(const MapEntry& lhs, const MapEntry& rhs) {
    return lhs.map->get_description() < rhs.map->get_description();

}
void MainMenu::fill_map_listbox(GamePlayType game_play_type) {
    const std::string& map_name = config.get_string("map_name");
    int select_index = 0;
    Resources::ResourceObjects& maps = resources.get_maps();
    MapEntries map_entries;
    size_t sz = maps.size();
    cs_maps->clear();

    for (size_t i = 0; i < sz; i++) {
        Map *map = static_cast<Map *>(maps[i].object);
        if (map_is_valid(game_play_type, map->get_game_play_type())) {
            map_entries.push_back(MapEntry(map, i));
        }
    }
    std::sort(map_entries.begin(), map_entries.end(), MapEntryCompare);

    for (MapEntries::iterator it = map_entries.begin(); it != map_entries.end(); it++) {
        MapEntry& map_entry = *it;
        cs_maps->add_entry(map_entry.map->get_description())->set_tag(map_entry.index);
        if (map_entry.map->get_name() == map_name) {
            select_index = static_cast<int>(cs_maps->get_entry_count() - 1);
        }
    }

    if (maps.size()) {
        map_selected(select_index);
        cs_maps->set_top_index(select_index);
        cs_maps->set_selected_index(select_index);
    }
}

void MainMenu::static_game_mode_click(GuiCheckbox *sender, void *data, bool state) {
    (reinterpret_cast<MainMenu *>(data))->game_mode_click(sender);
}

void MainMenu::game_mode_click(GuiCheckbox *sender) {
    cs_current_mode = sender;
    cs_dm->set_state(false);
    cs_tdm->set_state(false);
    cs_ctf->set_state(false);
    cs_sr->set_state(false);
    cs_ctc->set_state(false);
    cs_goh->set_state(false);
    sender->set_state(true);
    fill_map_listbox(static_cast<GamePlayType>(sender->get_tag()));
}

void MainMenu::static_map_selected(GuiListbox *sender, void *data, int index) {
    (reinterpret_cast<MainMenu *>(data))->map_selected(index);
}

void MainMenu::map_selected(int index) {
    int map_index = cs_maps->get_entry(index)->get_tag();
    Map *map = static_cast<Map *>(resources.get_maps()[map_index].object);
    cs_map_name->set_caption(map->get_name());
    Tile *preview = map->get_preview();
    if (preview) {
        cs_map_preview->set_picture(preview->get_tilegraphic());
    } else {
        cs_map_preview->set_picture(resources.get_icon("map_preview")->get_tile()->get_tilegraphic());
    }
}

void MainMenu::static_start_server_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->server_validate(false);
}

void MainMenu::static_close_start_server_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->server_validate(true);
}

void MainMenu::server_validate(bool close) {
    if (!cs_server_name->get_text().length()) {
        show_messagebox(Gui::MessageBoxIconError, "Server Name", "Please enter a valid server name.");
        cs_server_name->set_focus();
        return;
    }

    int port = atoi(cs_server_port->get_text().c_str());
    if (port < 1 || port > 65535) {
        show_messagebox(Gui::MessageBoxIconError, "Server Port", "Please enter a valid port number.");
        cs_server_port->set_focus();
        return;
    }

    int game_mode = static_cast<GamePlayType>(cs_current_mode->get_tag());

    int max_players = atoi(cs_max_players->get_text().c_str());
    if (max_players < 2 || max_players > 64) {
        show_messagebox(Gui::MessageBoxIconError, "Max. Players", "Please enter a valid number. (2 - 64)");
        cs_max_players->set_focus();
        return;
    }

    int duration = atoi(cs_duration->get_text().c_str());
    if (duration < 2 || duration > 240) {
        show_messagebox(Gui::MessageBoxIconError, "Duration", "Please enter a valid number. (2 - 240)");
        cs_duration->set_focus();
        return;
    }

    int warmup = atoi(cs_warmup->get_text().c_str());
    if (warmup < 0 || warmup > 60) {
        show_messagebox(Gui::MessageBoxIconError, "Warm Up", "Please enter a valid number. (0 - 60)");
        cs_warmup->set_focus();
        return;
    }

    int selected_map = cs_maps->get_entry(cs_maps->get_selected_index())->get_tag();
    Resources::ResourceObjects& maps = resources.get_maps();
    int sz = static_cast<int>(maps.size());
    if (selected_map < 0 || selected_map >= sz) {
        show_messagebox(Gui::MessageBoxIconError, "Select Map", "Please select a map.");
        cs_maps->set_focus();
        return;
    }
    Map *map = static_cast<Map *>(maps[selected_map].object);

    /* game mode map match */
    if (!map_is_valid(static_cast<GamePlayType>(game_mode), map->get_game_play_type())) {
        show_messagebox(Gui::MessageBoxIconError, "Wrong Map Type", "This map can't be played in this mode.");
        cs_maps->set_focus();
        return;
    }

    /* persist */
    config.set_string("server_name", cs_server_name->get_text());
    config.set_int("server_port", port);
    config.set_int("game_mode", game_mode);
    config.set_int("max_players", max_players);
    config.set_int("duration", duration);
    config.set_int("warmup", warmup);
    config.set_string("map_name", map->get_name());

    /* start server or close window */
    if (close) {
        pop_window();
    } else {
        try {
            ScopeMusicStopper stop_music(subsystem, resources.get_music(TitleMusic));
            GamePlayType type = static_cast<GamePlayType>(game_mode);
            Server server(resources, subsystem, port, max_players, config.get_string("server_name"),
                type, config.get_string("map_name"), duration, warmup);
            ScopeServer scope_server(server);
            Client client(resources, subsystem, INADDR_LOOPBACK, port, config, "");
            client.link_mouse(*this);
            client.run();
        } catch (const Exception& e) {
            show_messagebox(Gui::MessageBoxIconError, "Error", e.what());
        }
        fill_map_listbox(static_cast<GamePlayType>(cs_current_mode->get_tag()));
    }
}

bool MainMenu::map_is_valid(GamePlayType selected_type, GamePlayType map_type) {
    if (selected_type == GamePlayTypeSR || map_type == GamePlayTypeSR ||
        selected_type == GamePlayTypeCTC || map_type == GamePlayTypeCTC ||
        selected_type == GamePlayTypeGOH || map_type == GamePlayTypeGOH)
    {
        return (selected_type == map_type);
    }

    return (selected_type <= map_type);
}

/* ************************************************************************** */
/* List Packages                                                              */
/* ************************************************************************** */
void MainMenu::static_list_packages_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->list_packages_click();
}

void MainMenu::list_packages_click() {
    Font *f = get_font();
    int vw = subsystem.get_view_width();
    int vh = subsystem.get_view_height();
    int ww = 400;
    int wh = 256;
    int bh = 18;
    const int hash_width = 150;

    GuiWindow *window = push_window(vw / 2 - ww / 2, vh / 2 - wh / 2, ww, wh, "List Loaded Packages");
    window->set_cancelable(true);
    wh = window->get_client_height() + 2;

    GuiListbox *lb = create_listbox(window, Spc, Spc, ww - 2 * Spc, wh - 3 * Spc - bh, "Package", 0, 0);
    lb->show_title_bar(true);
    GuiListboxEntry *titlebar = lb->get_title_bar();
    titlebar->add_column("Hash", hash_width);

    const Resources::LoadedPaks loaded_paks = resources.get_loaded_paks();
    for (Resources::LoadedPaks::const_iterator it = loaded_paks.begin(); it != loaded_paks.end(); it++) {
        const Resources::LoadedPak& pak = *it;
        GuiListboxEntry *entry = lb->add_entry(pak.pak_short_name);
        entry->add_column(pak.pak_hash, hash_width);
    }
    lb->set_selected_index(0);

    /* close button */
    std::string text("Close");
    int bw = f->get_text_width(text) + 28;
    create_button(window, ww / 2 - bw / 2, wh - bh - Spc, bw, bh, text, static_close_packages_list_click, this);
}

void MainMenu::static_close_packages_list_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->close_packages_list_click();
}
void MainMenu::close_packages_list_click() {
    pop_window();
}

/* ************************************************************************** */
/* Options                                                                    */
/* ************************************************************************** */
void MainMenu::static_options_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->options_click();
}

void MainMenu::options_click() {
    show_options();
}

/* ************************************************************************** */
/* Credits                                                                    */
/* ************************************************************************** */
void MainMenu::static_credits_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->credits_click();
}

void MainMenu::credits_click() {
    int vw = subsystem.get_view_width();
    int vh = subsystem.get_view_height();
    int ww = 200;
    int wh = 240;
    int left = 10;
    int lft = 30;
    int tab = 90;

    GuiWindow *window = push_window(vw / 2 - ww / 2, vh / 2- wh / 2, ww, wh, "Credits");
    window->set_cancelable(true);

    create_label(window, lft, 10, "code:");
    create_label(window, tab, 10, "freanux");

    create_label(window, lft, 30, "graphics:");
    create_label(window, tab, 30, "freanux, ruby,");
    create_label(window, tab, 45, "cataclisma");

    create_label(window, lft, 65, "maps:");
    create_label(window, tab, 65, "ruby, freanux,");
    create_label(window, tab, 80, "cataclisma");

    create_label(window, lft, 100, "music:");
    create_label(window, tab, 100, "daniel wressle,");
    create_label(window, tab, 115, "martins garden");

    create_label(window, left, 145, "special thanks to:");
    create_label(window, left, 160, "ruby, cataclisma, julia, tanja, luxi");
    create_label(window, left, 175, "and of course, frederic the goat");

    int bw = 55;
    create_button(window, ww / 2 - bw / 2, wh - 43, bw, 18, "Close", static_close_window_click, this);
}

/* ************************************************************************** */
/* Quit                                                                       */
/* ************************************************************************** */
void MainMenu::static_quit_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<Gui *>(data))->leave();
}

/* ************************************************************************** */
/* Window close                                                               */
/* ************************************************************************** */
void MainMenu::static_close_window_click(GuiVirtualButton *sender, void *data) {
    (reinterpret_cast<MainMenu *>(data))->close_window_click();
}

void MainMenu::close_window_click() {
    pop_window();
}

/* ************************************************************************** */
/* Helpers                                                                    */
/* ************************************************************************** */
bool MainMenu::create_server_locators() throw (Exception) {
    destroy_server_locators();
    try {
        lan_broadcaster = new LANBroadcaster(config.get_int("server_port"));
    } catch (const Exception& e) {
        destroy_server_locators();
        show_messagebox(MessageBoxIconError, "Error", e.what());
        return false;
    }

    try {
        master_query = new MasterQuery(config.get_string("master_server"), config.get_int("master_port"));
    } catch (const Exception& e) {
        destroy_server_locators();
        show_messagebox(MessageBoxIconError, "Error", e.what());
        return false;
    }

    get_now(last_lan_info);
    return true;
}

void MainMenu::destroy_server_locators() {
    if (lan_broadcaster) {
        delete lan_broadcaster;
        lan_broadcaster = 0;
    }

    if (master_query) {
        delete master_query;
        master_query = 0;
    }
}
