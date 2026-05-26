#ifndef slic3r_GUI_FilamentVaultPanel_hpp_
#define slic3r_GUI_FilamentVaultPanel_hpp_

#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

#include "FilamentVault/Core/Spool.hpp"

class wxTextCtrl;
class Button;
class Label;

namespace FilamentVault {
    class SQLiteStore;
}

namespace Slic3r { namespace GUI {

class FilamentVaultPanel;

// -------------------------------------------------------------------
// FilamentCard — a single card drawn with wxDC
// -------------------------------------------------------------------
class FilamentCard : public wxPanel
{
public:
    FilamentCard(wxWindow *parent, const FilamentVault::Spool &spool, int index);

    int  spool_id() const { return m_spool.id; }
    int  index()    const { return m_index; }
    void setSelected(bool sel);
    bool isSelected() const { return m_selected; }

    std::function<void(int)> on_click;
    std::function<void(int)> on_dclick;

private:
    FilamentVault::Spool m_spool;
    int   m_index;
    bool  m_selected{false};
    bool  m_hovered{false};

    void OnPaint(wxPaintEvent &evt);
};

// -------------------------------------------------------------------
// FilamentVaultPanel — main panel with search, filters, card list
// -------------------------------------------------------------------
class FilamentVaultPanel : public wxPanel
{
public:
    FilamentVaultPanel(wxWindow *parent, wxWindowID id = wxID_ANY,
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize,
                       long style = wxTAB_TRAVERSAL);
    ~FilamentVaultPanel() override;

    void msw_rescale();
    void on_sys_color_changed();
    bool Show(bool show) override;

    bool apply_filament_for_printing(const FilamentVault::Spool &spool);
    void deduct_filament(double weight_g);

    int active_spool_id() const { return m_active_spool_id; }
    void set_active_spool_id(int id) { m_active_spool_id = id; }

private:
    void init_ui();
    void refresh_list();
    void sync_vault_to_presets();
    void apply_filters();
    void on_search();
    void select_card(int index);
    void on_add_spool();
    void on_edit_spool();
    void on_archive_spool();
    void on_print_with();

    FilamentVault::Spool *selected_spool();
    int selected_index();

    std::unique_ptr<FilamentVault::SQLiteStore> m_store;
    std::vector<FilamentVault::Spool>           m_spools;

    wxScrolledWindow *m_scroll{nullptr};
    wxBoxSizer       *m_card_sizer{nullptr};
    wxTextCtrl       *m_search{nullptr};
    Button           *m_add_btn{nullptr};
    Button           *m_edit_btn{nullptr};
    Button           *m_archive_btn{nullptr};
    Button           *m_print_btn{nullptr};
    Label            *m_count_lbl{nullptr};

    std::vector<Button *>   m_filter_btns;
    std::vector<FilamentCard *> m_cards;
    FilamentCard *m_sel_card{nullptr};

    std::string m_search_text;
    std::string m_active_filter;
    int m_active_spool_id{-1};
};

bool show_spool_dialog(wxWindow *parent, FilamentVault::Spool *spool);

}} // namespace Slic3r::GUI

#endif // slic3r_GUI_FilamentVaultPanel_hpp_
