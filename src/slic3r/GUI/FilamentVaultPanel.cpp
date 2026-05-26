#include "FilamentVaultPanel.hpp"

#include "GUI_App.hpp"
#include "GUI_Utils.hpp"
#include "I18N.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/StateColor.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "wxExtensions.hpp"

#include "FilamentVault/Core/Spool.hpp"
#include "FilamentVault/DB/SQLiteStore.hpp"

#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <set>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/numformatter.h>
#include <wx/filename.h>
#include <wx/dcgraph.h>
#include <wx/dcbuffer.h>
#include <wx/clrpicker.h>

namespace Slic3r { namespace GUI {

// ===================================================================
// FilamentCard
// ===================================================================
FilamentCard::FilamentCard(wxWindow *parent, const FilamentVault::Spool &spool, int index)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition,
              wxSize(-1, FromDIP(88)),
              wxBORDER_NONE | wxTAB_TRAVERSAL)
    , m_spool(spool), m_index(index)
{
    SetBackgroundColour(*wxWHITE);

    SetMinSize(wxSize(-1, FromDIP(88)));

    Bind(wxEVT_PAINT, &FilamentCard::OnPaint, this);
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent &) {
        m_hovered = true; Refresh();
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent &) {
        m_hovered = false; Refresh();
    });
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &) {
        if (on_click) on_click(m_index);
    });
    Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent &) {
        if (on_dclick) on_dclick(m_index);
    });
}

void FilamentCard::setSelected(bool sel)
{
    m_selected = sel;
    Refresh();
}

static bool is_dark()
{
    return StateColor::darkModeColorFor(wxColour("#FFFFFF")) != wxColour("#FFFFFF");
}

static wxColour cd(wxColour c)
{
    return StateColor::darkModeColorFor(c);
}

// ===================================================================
// Minimal outline icons for material badges (Lucide/Material style)
// ===================================================================
static void draw_icon_leaf(wxDC &dc, int cx, int cy, int s)
{
    int h = s / 2;
    int r = h * 4 / 5;
    // Two curves forming an almond/leaf shape + stem
    wxPoint pts[] = {
        {cx - h + r/3, cy + h/3},
        {cx,           cy - h + r/5},
        {cx + h - r/3, cy + h/3},
    };
    dc.DrawSpline(3, pts);
    dc.DrawLine(cx, cy - h + r/5, cx, cy - h - r/5);
}

static void draw_icon_drop(wxDC &dc, int cx, int cy, int s)
{
    int h = s / 2;
    // Circle top + converging sides to a point
    int rx = h * 2 / 3;
    int ry = h * 3 / 5;
    dc.DrawEllipticArc(cx - rx, cy - ry, rx * 2, ry * 2, 0, 180);
    dc.DrawLine(cx - rx, cy, cx, cy + h);
    dc.DrawLine(cx + rx, cy, cx, cy + h);
}

static void draw_icon_diamond(wxDC &dc, int cx, int cy, int s)
{
    int h = s / 2;
    wxPoint pts[] = {
        {cx,     cy - h},
        {cx + h, cy},
        {cx,     cy + h},
        {cx - h, cy},
    };
    dc.DrawLines(4, pts);
    dc.DrawLine(pts[3].x, pts[3].y, pts[0].x, pts[0].y);
}

static void draw_icon_wave(wxDC &dc, int cx, int cy, int s)
{
    int h = s / 2;
    int step = h * 2 / 3;
    wxPoint pts[] = {
        {cx - h, cy},
        {cx - h + step/2, cy - h/2},
        {cx - h + step,   cy + h/2},
        {cx - h + step*3/2, cy - h/2},
        {cx - h + step*2, cy},
        {cx - h + step*5/2, cy + h/2},
        {cx - h + step*3, cy},
    };
    dc.DrawLines(7, pts);
}

static void draw_icon_sun(wxDC &dc, int cx, int cy, int s)
{
    int h = s / 2;
    int r = h * 2 / 5;
    dc.DrawCircle(cx, cy, r);
    int ray = h - 2;
    for (int a = 0; a < 360; a += 72) {
        double rad = a * M_PI / 180.0;
        dc.DrawLine(
            cx + r * cos(rad), cy + r * sin(rad),
            cx + ray * cos(rad), cy + ray * sin(rad)
        );
    }
}

static void draw_icon_hexagon(wxDC &dc, int cx, int cy, int s)
{
    int h = s / 2;
    double step = M_PI / 3.0;
    wxPoint pts[6];
    for (int i = 0; i < 6; i++) {
        double a = -M_PI/2.0 + i * step;
        pts[i] = {cx + (int)(h * cos(a)), cy + (int)(h * sin(a))};
    }
    dc.DrawLines(6, pts);
    dc.DrawLine(pts[5].x, pts[5].y, pts[0].x, pts[0].y);
}

static void draw_icon_circle_dot(wxDC &dc, int cx, int cy, int s)
{
    int h = s / 2;
    int r = h * 2 / 3;
    dc.DrawCircle(cx, cy, r);
    dc.SetBrush(wxBrush(dc.GetPen().GetColour()));
    dc.DrawCircle(cx, cy, r / 3);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
}

static void draw_icon_star(wxDC &dc, int cx, int cy, int s)
{
    int h = s / 2;
    double step = M_PI / 5.0;
    wxPoint pts[10];
    for (int i = 0; i < 10; i++) {
        double a = -M_PI/2.0 + i * step;
        double r = (i % 2 == 0) ? h : h * 2 / 5;
        pts[i] = {cx + (int)(r * cos(a)), cy + (int)(r * sin(a))};
    }
    dc.DrawLines(10, pts);
    dc.DrawLine(pts[9].x, pts[9].y, pts[0].x, pts[0].y);
}

static void draw_material_icon(wxDC &dc, int cx, int cy, int sz, const std::string &material, const wxColour &color, int pen_w = 2)
{
    if (pen_w < 1) pen_w = 1;
    dc.SetPen(wxPen(color, pen_w));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    if (material.find("PLA") != std::string::npos)
        draw_icon_leaf(dc, cx, cy, sz);
    else if (material.find("PETG") != std::string::npos)
        draw_icon_drop(dc, cx, cy, sz);
    else if (material.find("ABS") != std::string::npos || material.find("ASA") != std::string::npos)
        draw_icon_diamond(dc, cx, cy, sz);
    else if (material.find("TPU") != std::string::npos || material.find("FLEX") != std::string::npos)
        draw_icon_wave(dc, cx, cy, sz);
    else if (material.find("PA") != std::string::npos)
        draw_icon_hexagon(dc, cx, cy, sz);
    else if (material.find("PC") != std::string::npos)
        draw_icon_circle_dot(dc, cx, cy, sz);
    else
        draw_icon_star(dc, cx, cy, sz);
}

void FilamentCard::OnPaint(wxPaintEvent &)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize sz = GetClientSize();
    bool dark = is_dark();

    int m = FromDIP(10);
    int r = FromDIP(8);

    // --- Background ---
    wxColour bg, border;
    if (m_selected) {
        bg     = dark ? wxColour("#1A3A4A")  : wxColour("#E3F2FD");
        border = dark ? wxColour("#42A5F5")  : wxColour("#1976D2");
    } else if (m_hovered) {
        bg     = dark ? wxColour("#36363C")  : wxColour("#F8F9FA");
        border = dark ? wxColour("#6B6B6B")  : wxColour("#BDBDBD");
    } else {
        bg     = cd(wxColour("#FFFFFF"));
        border = cd(wxColour("#EEEEEE"));
    }

    dc.SetBrush(wxBrush(bg));
    dc.SetPen(wxPen(border, 1));
    dc.DrawRoundedRectangle(1, 1, sz.x - 2, sz.y - 2, r);

    // --- Left color bar ---
    int bar_w = FromDIP(6);
    wxColour cc(m_spool.color_hex.empty() ? "#CCCCCC" : m_spool.color_hex);
    dc.SetBrush(wxBrush(cc));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRoundedRectangle(m, m, bar_w, sz.y - m * 2, FromDIP(3));

    // --- Material badge with icon (top-left of color bar area) ---
    int badge_x = m + bar_w + FromDIP(10);
    wxString mat_label = m_spool.material.substr(0, 4);

    wxColour mat_bg, mat_fg;
    if (m_spool.material.find("PETG") != std::string::npos) {
        mat_bg = dark ? wxColour("#3D2B1A") : wxColour("#FFF3E0");
        mat_fg = dark ? wxColour("#FF8A65") : wxColour("#E65100");
    } else if (m_spool.material.find("ABS") != std::string::npos ||
               m_spool.material.find("ASA") != std::string::npos) {
        mat_bg = dark ? wxColour("#3D1A1A") : wxColour("#FCE4EC");
        mat_fg = dark ? wxColour("#EF9A9A") : wxColour("#C62828");
    } else if (m_spool.material.find("TPU") != std::string::npos ||
               m_spool.material.find("FLEX") != std::string::npos) {
        mat_bg = dark ? wxColour("#1A3D3D") : wxColour("#E0F7FA");
        mat_fg = dark ? wxColour("#80DEEA") : wxColour("#00838F");
    } else {
        mat_bg = dark ? wxColour("#1B3B1B") : wxColour("#E8F5E9");
        mat_fg = dark ? wxColour("#66BB6A") : wxColour("#2E7D32");
    }

    int icon_sz = FromDIP(14);
    dc.SetFont(Label::Body_10);
    wxSize mat_sz = dc.GetTextExtent(mat_label);
    int badge_w = icon_sz + FromDIP(4) + mat_sz.x + FromDIP(8);
    int badge_h = mat_sz.y + FromDIP(4);
    if (badge_h < icon_sz + FromDIP(4))
        badge_h = icon_sz + FromDIP(4);
    dc.SetBrush(wxBrush(mat_bg));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRoundedRectangle(badge_x, m, badge_w, badge_h, FromDIP(3));

    // Draw material icon inside badge
    int icon_cx = badge_x + FromDIP(6) + icon_sz / 2;
    int icon_cy = m + badge_h / 2;
    draw_material_icon(dc, icon_cx, icon_cy, icon_sz, m_spool.material, mat_fg, std::max(1, FromDIP(2)));

    dc.SetTextForeground(mat_fg);
    dc.DrawText(mat_label, badge_x + FromDIP(6) + icon_sz + FromDIP(4), m + FromDIP(2));

    // --- Name ---
    int text_x = badge_x + badge_w + FromDIP(8);
    dc.SetFont(Label::Head_15);
    dc.SetTextForeground(cd(wxColour("#262E30")));
    wxString name = m_spool.name.empty() ? _L("Unnamed") : m_spool.name;
    dc.DrawText(name, text_x, m + FromDIP(1));

    // --- Brand · Material detail ---
    wxString detail;
    if (!m_spool.brand.empty())
        detail = m_spool.brand + "  \u00B7  " + m_spool.material;
    else
        detail = m_spool.material;

    dc.SetFont(Label::Body_12);
    dc.SetTextForeground(dark ? wxColour("#AAAAAA") : wxColour("#757575"));
    dc.DrawText(detail, text_x, m + FromDIP(24));

    // --- Remaining weight with icon (right side) ---
    wxString remain_str = wxString::Format("%.0f g", m_spool.weight_remain_g);
    int weight_icon_sz = FromDIP(14);
    int weight_icon_x = sz.x - m - weight_icon_sz;
    int weight_icon_y = m + FromDIP(3);
    dc.SetPen(wxPen(cd(wxColour("#262E30")), std::max(1, FromDIP(2))));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    // Draw a small scale icon: horizontal line with a hanging triangle
    int wh = weight_icon_sz / 2;
    dc.DrawLine(weight_icon_x, weight_icon_y + wh - 2, weight_icon_x + weight_icon_sz, weight_icon_y + wh - 2);
    dc.DrawLine(weight_icon_x + weight_icon_sz/2, weight_icon_y, weight_icon_x + weight_icon_sz/2, weight_icon_y + wh - 2);
    wxPoint tri[] = {
        {weight_icon_x + weight_icon_sz/2 - wh/2, weight_icon_y + wh - 2},
        {weight_icon_x + weight_icon_sz/2 + wh/2, weight_icon_y + wh - 2},
        {weight_icon_x + weight_icon_sz/2, weight_icon_y + wh + wh/2},
    };
    dc.DrawLines(3, tri);
    dc.DrawLine(tri[2].x, tri[2].y, tri[0].x, tri[0].y);

    dc.SetFont(Label::Head_15);
    dc.SetTextForeground(cd(wxColour("#262E30")));
    wxSize rem_sz = dc.GetTextExtent(remain_str);
    int rem_x = weight_icon_x - rem_sz.x - FromDIP(4);
    dc.DrawText(remain_str, rem_x, m + FromDIP(1));

    // --- Percentage ---
    double pct = m_spool.percentRemaining();
    wxString pct_str = wxString::Format("%.0f%%", pct);
    dc.SetFont(Label::Body_11);
    wxColour pct_col;
    if (pct > 50.0)
        pct_col = dark ? wxColour("#66BB6A") : wxColour("#4CAF50");
    else if (pct > 20.0)
        pct_col = dark ? wxColour("#FFA726") : wxColour("#FF9800");
    else
        pct_col = dark ? wxColour("#EF5350") : wxColour("#F44336");
    dc.SetTextForeground(pct_col);
    wxSize pct_sz = dc.GetTextExtent(pct_str);
    dc.DrawText(pct_str, sz.x - m - pct_sz.x, m + FromDIP(24));

    // --- Status badge (if low) ---
    if (pct < 20.0) {
        wxString warn = pct < 10.0 ? _L("CRITICAL") : _L("LOW");
        wxColour warn_bg, warn_fg;
        if (pct < 10.0) {
            warn_bg = dark ? wxColour("#3D1A1A") : wxColour("#FFEBEE");
            warn_fg = dark ? wxColour("#EF9A9A") : wxColour("#C62828");
        } else {
            warn_bg = dark ? wxColour("#3D2B1A") : wxColour("#FFF8E1");
            warn_fg = dark ? wxColour("#FFB74D") : wxColour("#F57F17");
        }
        // Draw small warning icon inside badge
        int warn_icon_sz = FromDIP(12);
        dc.SetFont(Label::Body_10);
        wxSize warn_sz = dc.GetTextExtent(warn);
        int warn_w = warn_icon_sz + FromDIP(4) + warn_sz.x + FromDIP(8);
        int warn_h = warn_sz.y + FromDIP(4);
        if (warn_h < warn_icon_sz + FromDIP(4))
            warn_h = warn_icon_sz + FromDIP(4);
        dc.SetBrush(wxBrush(warn_bg));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRoundedRectangle(text_x, m + FromDIP(44), warn_w, warn_h, FromDIP(3));

        // Warning triangle icon
        int wicx = text_x + FromDIP(6) + warn_icon_sz / 2;
        int wicy = m + FromDIP(44) + warn_h / 2;
        int wh2 = warn_icon_sz / 2;
        dc.SetPen(wxPen(warn_fg, std::max(1, FromDIP(2))));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        // Triangle
        wxPoint tri[] = {
            {wicx, wicy - wh2},
            {wicx + wh2, wicy + wh2},
            {wicx - wh2, wicy + wh2},
        };
        dc.DrawLines(3, tri);
        dc.DrawLine(tri[2].x, tri[2].y, tri[0].x, tri[0].y);
        // Exclamation mark
        int ex_w = std::max(1, FromDIP(2));
        dc.SetPen(wxPen(warn_fg, ex_w));
        dc.DrawLine(wicx, wicy - wh2/2, wicx, wicy + wh2/4);
        dc.DrawCircle(wicx, wicy + wh2*3/5, std::max(1, FromDIP(1)));

        dc.SetFont(Label::Body_10);
        dc.SetTextForeground(warn_fg);
        dc.DrawText(warn, text_x + FromDIP(6) + warn_icon_sz + FromDIP(4), m + FromDIP(46));
    }

    // --- Progress bar ---
    int bar_y = sz.y - m - FromDIP(8);
    int bar_h = FromDIP(5);
    int bar_x = text_x;
    int bar_w_total = sz.x - text_x - m - FromDIP(60);

    if (bar_w_total > FromDIP(20)) {
        dc.SetBrush(wxBrush(dark ? wxColour("#3E3E45") : wxColour("#E0E0E0")));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRoundedRectangle(bar_x, bar_y, bar_w_total, bar_h, FromDIP(2));

        if (pct > 0.0) {
            int fill_w = static_cast<int>(bar_w_total * pct / 100.0);
            if (fill_w > 0) {
                wxColour fill;
                if (pct > 50.0)
                    fill = dark ? wxColour("#66BB6A") : wxColour("#4CAF50");
                else if (pct > 20.0)
                    fill = dark ? wxColour("#FFA726") : wxColour("#FF9800");
                else
                    fill = dark ? wxColour("#EF5350") : wxColour("#F44336");
                dc.SetBrush(wxBrush(fill));
                dc.DrawRoundedRectangle(bar_x, bar_y, fill_w, bar_h, FromDIP(2));
            }
        }
    }
}

// ===================================================================
// FilamentVaultPanel
// ===================================================================
FilamentVaultPanel::FilamentVaultPanel(wxWindow *parent, wxWindowID id,
                                       const wxPoint &pos, const wxSize &size,
                                       long style)
    : wxPanel(parent, id, pos, size, style)
{
    SetBackgroundColour(*wxWHITE);
    wxGetApp().UpdateDarkUI(this);

    try {
        auto db_path = wxFileName(wxString(Slic3r::data_dir()), "filament_vault.db");
        m_store      = std::make_unique<FilamentVault::SQLiteStore>(db_path.GetFullPath().ToStdString());
    } catch (const std::exception &e) {
        m_store = nullptr;
    }

    init_ui();
}

FilamentVaultPanel::~FilamentVaultPanel() = default;

// -------------------------------------------------------------------
// init_ui
// -------------------------------------------------------------------
void FilamentVaultPanel::init_ui()
{
    auto *outer = new wxBoxSizer(wxVERTICAL);

    // === Top bar: title + search + add btn ===
    auto *top = new wxBoxSizer(wxHORIZONTAL);

    auto *title = new Label(this, _L("Filament Vault"));
    title->SetFont(Label::Head_18);
    top->Add(title, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(16));

    // Search with magnifier icon
    auto *search_icon = new wxStaticBitmap(this, wxID_ANY,
        create_scaled_bitmap("search", this, FromDIP(16)));
    top->Add(search_icon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));

    m_search = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                              wxDefaultPosition, wxSize(FromDIP(240), FromDIP(32)),
                              wxTE_PROCESS_ENTER);
    m_search->SetHint(_L("Search spools..."));
    m_search->SetFont(Label::Body_13);
    wxGetApp().UpdateDarkUI(m_search);
    top->Add(m_search, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

    m_add_btn = new Button(this, _L("Add Spool"), "add_filament", 0, FromDIP(16));
    m_add_btn->SetStyle(ButtonStyle::Confirm, ButtonType::Choice);
    m_add_btn->SetMinSize(wxSize(FromDIP(140), FromDIP(32)));
    wxGetApp().UpdateDarkUI(m_add_btn);
    top->Add(m_add_btn, 0, wxALIGN_CENTER_VERTICAL);

    outer->Add(top, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(16));

    // === Filter chips ===
    auto *filter_sizer = new wxBoxSizer(wxHORIZONTAL);

    auto add_filter = [&](const wxString &label, const std::string &mat) {
        auto *btn = new Button(this, label, "filament", 0, FromDIP(14));
        btn->SetStyle(ButtonStyle::Regular, ButtonType::Compact);
        btn->SetMinSize(wxSize(-1, FromDIP(26)));
        btn->Bind(wxEVT_BUTTON, [this, mat](wxCommandEvent &) {
            m_active_filter = mat;
            for (auto *b : m_filter_btns) {
                bool sel = (b->GetLabel().ToStdString() ==
                           (mat.empty() ? std::string("All") : mat));
                b->SetSelected(sel);
            }
            apply_filters();
        });
        wxGetApp().UpdateDarkUI(btn);
        m_filter_btns.push_back(btn);
        filter_sizer->Add(btn, 0, wxRIGHT, FromDIP(4));
    };

    add_filter(_L("All"),    "");
    add_filter("PLA",        "PLA");
    add_filter("PETG",       "PETG");
    add_filter("ABS",        "ABS");
    add_filter("TPU",        "TPU");
    add_filter("ASA",        "ASA");
    add_filter("PA",         "PA");
    add_filter("PC",         "PC");
    add_filter("Others",     "__other__");
    // Select "All" by default
    if (!m_filter_btns.empty()) m_filter_btns[0]->SetSelected(true);

    outer->Add(filter_sizer, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
    outer->AddSpacer(FromDIP(8));

    // === Card scroll area ===
    bool dark = is_dark();
    m_scroll = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxVSCROLL | wxBORDER_NONE);
    m_scroll->SetBackgroundColour(dark ? wxColour("#333338") : wxColour("#F5F5F5"));
    m_scroll->SetForegroundColour(dark ? wxColour("#EFEFF0") : wxColour("#262E30"));
    m_scroll->SetScrollRate(0, FromDIP(10));

    m_card_sizer = new wxBoxSizer(wxVERTICAL);
    m_scroll->SetSizer(m_card_sizer);

    outer->Add(m_scroll, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(4));

    // === Bottom action bar ===
    auto *action_bar = new wxBoxSizer(wxHORIZONTAL);

    m_edit_btn    = new Button(this, _L("Edit Selected"), "edit", 0, FromDIP(16));
    m_archive_btn = new Button(this, _L("Archive Selected"), "auxiliary_delete", 0, FromDIP(16));

    for (auto *btn : {m_edit_btn, m_archive_btn}) {
        btn->SetStyle(ButtonStyle::Regular, ButtonType::Choice);
        btn->Disable();
        wxGetApp().UpdateDarkUI(btn);
    }

    m_print_btn = new Button(this, _L("Print with this"), "print_info_weight", 0, FromDIP(16));
    m_print_btn->SetStyle(ButtonStyle::Regular, ButtonType::Choice);
    m_print_btn->Disable();
    wxGetApp().UpdateDarkUI(m_print_btn);

    action_bar->Add(m_edit_btn, 0, wxRIGHT, FromDIP(6));
    action_bar->Add(m_archive_btn, 0, wxRIGHT, FromDIP(6));
    action_bar->Add(m_print_btn, 0, wxRIGHT, FromDIP(6));
    action_bar->AddStretchSpacer();

    // Show total count
    m_count_lbl = new Label(this, wxEmptyString);
    m_count_lbl->SetFont(Label::Body_12);
    m_count_lbl->SetForegroundColour(cd(wxColour("#9E9E9E")));
    action_bar->Add(m_count_lbl, 0, wxALIGN_CENTER_VERTICAL);

    outer->Add(action_bar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, FromDIP(12));

    SetSizerAndFit(outer);
    Layout();
    wxGetApp().UpdateDarkUIWin(this);

    // --- Bind events ---
    m_search->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent &) { on_search(); });
    m_search->Bind(wxEVT_TEXT, [this](wxCommandEvent &) {
        m_search_text = m_search->GetValue().ToStdString();
        apply_filters();
    });

    m_add_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { on_add_spool(); });
    m_edit_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { on_edit_spool(); });
    m_archive_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { on_archive_spool(); });
    m_print_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { on_print_with(); });

    refresh_list();
}

// -------------------------------------------------------------------
// refresh_list
// -------------------------------------------------------------------
void FilamentVaultPanel::refresh_list()
{
    if (!m_store) return;
    m_spools = m_store->listSpools(false);
    sync_vault_to_presets();
    apply_filters();
}

// -------------------------------------------------------------------
// apply_filters — rebuilds card list from m_spools + m_search_text + m_active_filter
// -------------------------------------------------------------------
void FilamentVaultPanel::apply_filters()
{
    if (!m_scroll || !m_card_sizer) return;

    // --- Build filtered list ---
    std::vector<FilamentVault::Spool> filtered;
    for (const auto &s : m_spools) {
        // Material filter
        if (!m_active_filter.empty()) {
            if (m_active_filter == "__other__") {
                static const char *known[] = {"PLA","PETG","ABS","ASA","TPU","PA","PC","PP","PE","HIPS","PVA","BVOH","PVB"};
                bool known_mat = false;
                for (auto &k : known) {
                    if (s.material.find(k) != std::string::npos) {
                        known_mat = true;
                        break;
                    }
                }
                if (known_mat) continue;
            } else {
                if (s.material.find(m_active_filter) == std::string::npos)
                    continue;
            }
        }

        // Search text
        if (!m_search_text.empty()) {
            auto q = m_search_text;
            std::transform(q.begin(), q.end(), q.begin(), ::tolower);
            std::string lower_name = s.name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
            std::string lower_brand = s.brand;
            std::transform(lower_brand.begin(), lower_brand.end(), lower_brand.begin(), ::tolower);
            std::string lower_mat = s.material;
            std::transform(lower_mat.begin(), lower_mat.end(), lower_mat.begin(), ::tolower);
            if (lower_name.find(q) == std::string::npos &&
                lower_brand.find(q) == std::string::npos &&
                lower_mat.find(q) == std::string::npos)
                continue;
        }

        filtered.push_back(s);
    }

    // --- Rebuild cards ---
    m_sel_card = nullptr;

    // Freeze to avoid flicker
    m_scroll->Freeze();

    for (auto *card : m_cards)
        card->Destroy();
    m_cards.clear();
    m_card_sizer->Clear(false);

    for (size_t i = 0; i < filtered.size(); ++i) {
        FilamentCard *card = new FilamentCard(m_scroll, filtered[i], static_cast<int>(i));
        card->on_click  = [this](int idx) { select_card(idx); };
        card->on_dclick = [this](int) { on_edit_spool(); };
        m_cards.push_back(card);
        m_card_sizer->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(4));
    }

    // Empty state
    if (filtered.empty()) {
        auto *empty_lbl = new Label(m_scroll, _L("No spools found."));
        empty_lbl->SetFont(Label::Body_14);
        empty_lbl->SetForegroundColour(cd(wxColour("#9E9E9E")));
        m_card_sizer->AddStretchSpacer();
        m_card_sizer->Add(empty_lbl, 0, wxALIGN_CENTER);
        m_card_sizer->AddStretchSpacer();
    }

    m_scroll->Thaw();
    m_scroll->FitInside();

    // Update count label
    if (m_count_lbl) {
        if (m_spools.empty()) {
            m_count_lbl->SetLabel(wxString::Format(_L("%zu spools"), filtered.size()));
        } else if (filtered.size() == m_spools.size()) {
            m_count_lbl->SetLabel(wxString::Format(_L("%zu spools"), m_spools.size()));
        } else {
            m_count_lbl->SetLabel(wxString::Format(_L("%zu / %zu filtered"), filtered.size(), m_spools.size()));
        }
    }
}

static Preset *find_base_preset(PresetBundle *, const FilamentVault::Spool &);

// -------------------------------------------------------------------
// sync_vault_to_presets — make vault spools selectable in Plater sidebar
// -------------------------------------------------------------------
void FilamentVaultPanel::sync_vault_to_presets()
{
    auto *bundle = wxGetApp().preset_bundle;
    if (!bundle || !m_store) return;

    auto spools = m_store->listSpools(false);

    // Collect existing vault-derived preset names for cleanup
    std::set<std::string> vault_preset_names;
    std::string prefix = "Vault: ";

    for (const auto &spool : spools) {
        std::string name = prefix + spool.name;
        if (!spool.material.empty())
            name += " (" + spool.material + ")";
        vault_preset_names.insert(name);

        // Skip if preset already exists
        if (bundle->filaments.find_preset(name, false))
            continue;

        // Build config from best matching base preset, or default
        DynamicPrintConfig cfg;
        if (Preset *base = find_base_preset(bundle, spool))
            cfg = base->config;
        else
            cfg = bundle->filaments.default_preset().config;

        auto set_str = [](ConfigOptionStrings *opt, const std::string &val) {
            if (!opt) return;
            if (opt->values.empty()) opt->values.emplace_back(val);
            else opt->values[0] = val;
        };
        auto set_flt = [](ConfigOptionFloats *opt, double val) {
            if (!opt) return;
            if (opt->values.empty()) opt->values.emplace_back(val);
            else opt->values[0] = val;
        };
        auto set_int = [](ConfigOptionInts *opt, int val) {
            if (!opt) return;
            if (opt->values.empty()) opt->values.emplace_back(val);
            else opt->values[0] = val;
        };
        set_str(cfg.option<ConfigOptionStrings>("filament_vendor"),    spool.brand.empty() ? "Generic" : spool.brand);
        set_str(cfg.option<ConfigOptionStrings>("filament_type"),      spool.material.empty() ? "PLA" : spool.material);
        set_str(cfg.option<ConfigOptionStrings>("filament_color"),     spool.color_hex.empty() ? "#FFFFFF" : spool.color_hex);
        set_str(cfg.option<ConfigOptionStrings>("compatible_printers"), "*");
        set_flt(cfg.option<ConfigOptionFloats>("filament_diameter"),   spool.diameter_mm);
        set_flt(cfg.option<ConfigOptionFloats>("filament_density"),    spool.density_g_cm3.value_or(1.24));
        if (spool.nozzle_temp_c > 0)
            set_int(cfg.option<ConfigOptionInts>("nozzle_temperature"), spool.nozzle_temp_c);
        if (spool.nozzle_temp_initial_c > 0)
            set_int(cfg.option<ConfigOptionInts>("nozzle_temperature_initial_layer"), spool.nozzle_temp_initial_c);
        if (spool.bed_temp_c > 0)
            set_int(cfg.option<ConfigOptionInts>("bed_temperature"), spool.bed_temp_c);
        if (spool.bed_temp_initial_c > 0)
            set_int(cfg.option<ConfigOptionInts>("bed_temperature_initial_layer"), spool.bed_temp_initial_c);

        bundle->filaments.load_preset("", name, cfg, false);
    }

    // Remove vault presets for spools that no longer exist
    for (auto it = bundle->filaments.begin(); it != bundle->filaments.end(); ) {
        if (it->name.compare(0, prefix.size(), prefix) == 0 &&
            vault_preset_names.find(it->name) == vault_preset_names.end()) {
            it = bundle->filaments.erase(it);
        } else {
            ++it;
        }
    }
}

// -------------------------------------------------------------------
// search / filter
// -------------------------------------------------------------------
void FilamentVaultPanel::on_search()
{
    m_search_text = m_search->GetValue().ToStdString();
    apply_filters();
}

void FilamentVaultPanel::select_card(int index)
{
    if (m_sel_card) {
        m_sel_card->setSelected(false);
    }
    if (index >= 0 && index < static_cast<int>(m_cards.size())) {
        m_sel_card = m_cards[index];
        m_sel_card->setSelected(true);
        m_edit_btn->Enable();
        m_archive_btn->Enable();
        m_print_btn->Enable();
    } else {
        m_sel_card = nullptr;
        m_edit_btn->Disable();
        m_archive_btn->Disable();
        m_print_btn->Disable();
    }
}

// -------------------------------------------------------------------
// CRUD operations
// -------------------------------------------------------------------
int FilamentVaultPanel::selected_index()
{
    if (!m_sel_card) return -1;
    // Find the spool index in m_spools by ID
    int id = m_sel_card->spool_id();
    for (size_t i = 0; i < m_spools.size(); ++i) {
        if (m_spools[i].id == id)
            return static_cast<int>(i);
    }
    return -1;
}

FilamentVault::Spool *FilamentVaultPanel::selected_spool()
{
    int idx = selected_index();
    return idx >= 0 ? &m_spools[idx] : nullptr;
}

void FilamentVaultPanel::on_add_spool()
{
    if (!m_store) return;
    FilamentVault::Spool s;
    if (!show_spool_dialog(this, &s))
        return;
    if (m_store->insertSpool(s).has_value())
        refresh_list();
    else
        wxMessageBox(_L("Failed to add spool."), _L("Error"), wxOK | wxICON_ERROR, this);
}

void FilamentVaultPanel::on_edit_spool()
{
    if (!m_store) return;
    auto *s = selected_spool();
    if (!s) return;
    FilamentVault::Spool copy = *s;
    if (!show_spool_dialog(this, &copy))
        return;
    copy.id = s->id;
    if (m_store->updateSpool(copy))
        refresh_list();
    else
        wxMessageBox(_L("Failed to update spool."), _L("Error"), wxOK | wxICON_ERROR, this);
}

void FilamentVaultPanel::on_archive_spool()
{
    if (!m_store) return;
    auto *s = selected_spool();
    if (!s) return;
    auto ans = wxMessageBox(
        wxString::Format(_L("Archive \"%s\" (%s)?"), s->name, s->material),
        _L("Archive Spool"), wxYES_NO | wxICON_QUESTION, this);
    if (ans != wxYES) return;
    if (m_store->archiveSpool(s->id))
        refresh_list();
    else
        wxMessageBox(_L("Failed to archive spool."), _L("Error"), wxOK | wxICON_ERROR, this);
}

void FilamentVaultPanel::on_print_with()
{
    auto *s = selected_spool();
    if (!s) return;
    m_active_spool_id = s->id;
    apply_filament_for_printing(*s);
    wxMessageBox(
        wxString::Format(_L("Filament profile created for \"%s\" (%s) and set as active."), s->name, s->material),
        _L("Print Filament"), wxOK | wxICON_INFORMATION, this);
}

void FilamentVaultPanel::deduct_filament(double weight_g)
{
    if (!m_store || m_active_spool_id < 0 || weight_g <= 0.0)
        return;
    m_store->deductFilament(m_active_spool_id, weight_g);
    refresh_list();
}

// Find the best base preset: try brand+material first, then material-only, then generic
static Preset *find_base_preset(PresetBundle *bundle, const FilamentVault::Spool &spool)
{
    auto ci_eq = [](const std::string &a, const std::string &b) -> bool {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); i++)
            if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i]))
                return false;
        return true;
    };

    // Try brand + material match
    for (Preset &preset : bundle->filaments) {
        auto *vendor = preset.config.option<ConfigOptionStrings>("filament_vendor");
        auto *type   = preset.config.option<ConfigOptionStrings>("filament_type");
        if (!vendor || vendor->values.empty() || !type || type->values.empty())
            continue;
        if (ci_eq(vendor->values[0], spool.brand) && ci_eq(type->values[0], spool.material))
            return &preset;
    }
    // Try material-only match (e.g., "Generic PLA")
    for (Preset &preset : bundle->filaments) {
        auto *type = preset.config.option<ConfigOptionStrings>("filament_type");
        if (!type || type->values.empty())
            continue;
        if (ci_eq(type->values[0], spool.material))
            return &preset;
    }
    return nullptr;
}

bool FilamentVaultPanel::apply_filament_for_printing(const FilamentVault::Spool &spool)
{
    auto *bundle = wxGetApp().preset_bundle;
    if (!bundle) return false;

    // Build the vault preset name
    std::string name = "Vault: " + spool.name;
    if (!spool.material.empty())
        name += " (" + spool.material + ")";

    // Create or update the preset
    Preset *existing = bundle->filaments.find_preset(name, false);
    DynamicPrintConfig cfg;
    if (existing) {
        cfg = existing->config;
    } else if (Preset *base = find_base_preset(bundle, spool)) {
        cfg = base->config;
    } else {
        cfg = bundle->filaments.default_preset().config;
    }

    auto set_str = [](ConfigOptionStrings *opt, const std::string &val) {
        if (!opt) return;
        if (opt->values.empty()) opt->values.emplace_back(val);
        else opt->values[0] = val;
    };
    auto set_flt = [](ConfigOptionFloats *opt, double val) {
        if (!opt) return;
        if (opt->values.empty()) opt->values.emplace_back(val);
        else opt->values[0] = val;
    };
    auto set_int = [](ConfigOptionInts *opt, int val) {
        if (!opt) return;
        if (opt->values.empty()) opt->values.emplace_back(val);
        else opt->values[0] = val;
    };
    set_str(cfg.option<ConfigOptionStrings>("filament_vendor"),    spool.brand.empty() ? "Generic" : spool.brand);
    set_str(cfg.option<ConfigOptionStrings>("filament_type"),      spool.material.empty() ? "PLA" : spool.material);
    set_str(cfg.option<ConfigOptionStrings>("filament_color"),     spool.color_hex.empty() ? "#FFFFFF" : spool.color_hex);
    set_str(cfg.option<ConfigOptionStrings>("compatible_printers"), "*");
    set_flt(cfg.option<ConfigOptionFloats>("filament_diameter"),   spool.diameter_mm);
    set_flt(cfg.option<ConfigOptionFloats>("filament_density"),    spool.density_g_cm3.value_or(1.24));
    if (spool.nozzle_temp_c > 0)
        set_int(cfg.option<ConfigOptionInts>("nozzle_temperature"), spool.nozzle_temp_c);
    if (spool.nozzle_temp_initial_c > 0)
        set_int(cfg.option<ConfigOptionInts>("nozzle_temperature_initial_layer"), spool.nozzle_temp_initial_c);
    if (spool.bed_temp_c > 0)
        set_int(cfg.option<ConfigOptionInts>("bed_temperature"), spool.bed_temp_c);
    if (spool.bed_temp_initial_c > 0)
        set_int(cfg.option<ConfigOptionInts>("bed_temperature_initial_layer"), spool.bed_temp_initial_c);

    bundle->filaments.load_preset("", name, cfg, existing != nullptr);
    bundle->set_filament_preset(0, name);
    return true;
}

// -------------------------------------------------------------------
// misc overrides
// -------------------------------------------------------------------
void FilamentVaultPanel::msw_rescale()
{
    wxGetApp().UpdateDarkUIWin(this);
    for (auto *card : m_cards)
        card->Refresh();
}

void FilamentVaultPanel::on_sys_color_changed()
{
    bool dark = is_dark();
    m_scroll->SetBackgroundColour(dark ? wxColour("#333338") : wxColour("#F5F5F5"));
    m_scroll->SetForegroundColour(dark ? wxColour("#EFEFF0") : wxColour("#262E30"));
    m_count_lbl->SetForegroundColour(cd(wxColour("#9E9E9E")));
    wxGetApp().UpdateDarkUIWin(this);
    for (auto *card : m_cards)
        card->Refresh();
}

bool FilamentVaultPanel::Show(bool show)
{
    if (show)
        refresh_list();
    return wxPanel::Show(show);
}

// ===================================================================
// Spool edit dialog (with brand/material dropdowns from Orca presets)
// ===================================================================
bool show_spool_dialog(wxWindow *parent, FilamentVault::Spool *spool)
{
    wxDialog dlg(parent, wxID_ANY,
                 spool->id < 0 ? _L("Add Spool") : _L("Edit Spool"),
                 wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    dlg.SetBackgroundColour(*wxWHITE);
    wxGetApp().UpdateDarkUI(&dlg);

    auto *main     = new wxBoxSizer(wxVERTICAL);
    auto add_row   = [&](const wxString &label, wxWindow *ctrl) {
        auto *row = new wxBoxSizer(wxHORIZONTAL);
        auto *st  = new wxStaticText(&dlg, wxID_ANY, label);
        st->SetFont(Label::Body_13);
        row->Add(st, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dlg.FromDIP(8));
        row->Add(ctrl, 1, wxEXPAND);
        main->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, dlg.FromDIP(4));
    };

    // Collect brand/material options from OrcaSlicer filament presets
    std::vector<wxString> brand_options, material_options;
    auto *bundle = wxGetApp().preset_bundle;
    if (bundle) {
        std::set<std::string> brands, types;
        for (const Preset &preset : bundle->filaments) {
            if (auto *v = preset.config.option<ConfigOptionStrings>("filament_vendor")) {
                if (!v->values.empty()) brands.insert(v->values[0]);
            }
            if (auto *t = preset.config.option<ConfigOptionStrings>("filament_type")) {
                if (!t->values.empty()) types.insert(t->values[0]);
            }
        }
        brand_options.reserve(brands.size() + 1);
        brand_options.emplace_back(wxEmptyString);
        for (auto &b : brands) brand_options.emplace_back(b);

        material_options.reserve(types.size() + 1);
        material_options.emplace_back(wxEmptyString);
        for (auto &t : types) material_options.emplace_back(t);
    }

    // Sort by name
    auto sort_skip_first = [](std::vector<wxString> &v) {
        if (v.size() > 1) std::sort(v.begin() + 1, v.end());
    };
    sort_skip_first(brand_options);
    sort_skip_first(material_options);

    auto *name_txt     = new wxTextCtrl(&dlg, wxID_ANY, spool->name);

    // Brand: editable combo with all preset vendors
    auto *brand_combo  = new wxComboBox(&dlg, wxID_ANY, spool->brand,
                                         wxDefaultPosition, wxDefaultSize,
                                         (int)brand_options.size(), brand_options.data(),
                                         wxCB_DROPDOWN);
    // Material: editable combo with all preset types
    auto *material_combo = new wxComboBox(&dlg, wxID_ANY, spool->material,
                                           wxDefaultPosition, wxDefaultSize,
                                           (int)material_options.size(), material_options.data(),
                                           wxCB_DROPDOWN);

    auto *color_txt    = new wxTextCtrl(&dlg, wxID_ANY, spool->color_name);
    auto *color_picker = new wxColourPickerCtrl(&dlg, wxID_ANY,
        wxColour(spool->color_hex.empty() ? "#CCCCCC" : spool->color_hex));

    auto  fmt_double   = [](double v, int prec) {
        return wxNumberFormatter::ToString(v, prec);
    };
    auto *diam_txt     = new wxTextCtrl(&dlg, wxID_ANY, fmt_double(spool->diameter_mm, 2));
    auto *total_txt    = new wxTextCtrl(&dlg, wxID_ANY, fmt_double(spool->weight_total_g, 1));
    auto *remain_txt   = new wxTextCtrl(&dlg, wxID_ANY, fmt_double(spool->weight_remain_g, 1));
    auto *density_txt  = new wxTextCtrl(&dlg, wxID_ANY,
        spool->density_g_cm3.has_value() ? fmt_double(spool->density_g_cm3.value(), 2) : wxString(""));
    auto *nozzle_txt   = new wxTextCtrl(&dlg, wxID_ANY, spool->nozzle_temp_c > 0 ? fmt_double(spool->nozzle_temp_c, 0) : wxString(""));
    auto *nozzle_init_txt = new wxTextCtrl(&dlg, wxID_ANY, spool->nozzle_temp_initial_c > 0 ? fmt_double(spool->nozzle_temp_initial_c, 0) : wxString(""));
    auto *bed_txt      = new wxTextCtrl(&dlg, wxID_ANY, spool->bed_temp_c > 0 ? fmt_double(spool->bed_temp_c, 0) : wxString(""));
    auto *bed_init_txt = new wxTextCtrl(&dlg, wxID_ANY, spool->bed_temp_initial_c > 0 ? fmt_double(spool->bed_temp_initial_c, 0) : wxString(""));
    auto *notes_txt    = new wxTextCtrl(&dlg, wxID_ANY, spool->notes,
                                         wxDefaultPosition, wxSize(dlg.FromDIP(200), dlg.FromDIP(60)),
                                         wxTE_MULTILINE);

    add_row(_L("Name") + ":",     name_txt);
    add_row(_L("Brand") + ":",    brand_combo);
    add_row(_L("Material") + ":", material_combo);
    add_row(_L("Color") + ":",    color_txt);
    add_row(_L("Hex") + ":",      color_picker);
    add_row(_L("Diameter (mm)") + ":",  diam_txt);
    add_row(_L("Total (g)") + ":",      total_txt);
    add_row(_L("Remaining (g)") + ":",  remain_txt);
    add_row(_L("Density (g/cm\xc2\xb3)") + ":", density_txt);

    // Do the test! section
    auto *test_label = new wxStaticText(&dlg, wxID_ANY, _L("Do the test!"));
    test_label->SetFont(Label::Head_15);
    main->AddSpacer(dlg.FromDIP(8));
    main->Add(test_label, 0, wxLEFT | wxRIGHT, dlg.FromDIP(4));
    main->AddSpacer(dlg.FromDIP(4));

    add_row(_L("Hotend temp (\u00B0C)") + ":",            nozzle_txt);
    add_row(_L("Hotend initial layer (\u00B0C)") + ":",   nozzle_init_txt);
    add_row(_L("Bed temp (\u00B0C)") + ":",               bed_txt);
    add_row(_L("Bed initial layer (\u00B0C)") + ":",      bed_init_txt);

    // Apply-as-print-filament checkbox
    auto *apply_chk = new wxCheckBox(&dlg, wxID_ANY, _L("Set as active print filament"));
    apply_chk->SetFont(Label::Body_13);
    apply_chk->SetValue(true);
    main->AddSpacer(dlg.FromDIP(4));
    main->Add(apply_chk, 0, wxLEFT | wxRIGHT, dlg.FromDIP(4));

    main->AddSpacer(dlg.FromDIP(4));
    auto *notes_label = new wxStaticText(&dlg, wxID_ANY, _L("Notes") + ":");
    notes_label->SetFont(Label::Body_13);
    main->Add(notes_label, 0, wxLEFT | wxRIGHT | wxTOP, dlg.FromDIP(4));
    main->Add(notes_txt, 1, wxEXPAND | wxLEFT | wxRIGHT, dlg.FromDIP(4));

    main->AddSpacer(dlg.FromDIP(8));
    auto *btn_sizer = dlg.CreateButtonSizer(wxOK | wxCANCEL);
    if (auto *ok = dlg.FindWindow(wxID_OK)) {
        ok->SetLabel(_L("Save"));
        wxGetApp().UpdateDarkUI(ok);
    }
    if (auto *cancel = dlg.FindWindow(wxID_CANCEL)) {
        cancel->SetLabel(_L("Cancel"));
        wxGetApp().UpdateDarkUI(cancel);
    }
    main->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dlg.FromDIP(8));

    dlg.SetSizer(main);
    main->SetSizeHints(&dlg);
    dlg.SetMinSize(wxSize(dlg.FromDIP(440), dlg.FromDIP(600)));

    if (dlg.ShowModal() != wxID_OK)
        return false;

    spool->name       = name_txt->GetValue().ToStdString();
    spool->brand      = brand_combo->GetValue().ToStdString();
    spool->material   = material_combo->GetValue().ToStdString();
    spool->color_name = color_txt->GetValue().ToStdString();
    spool->color_hex  = color_picker->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();

    double d;
    diam_txt->GetValue().ToDouble(&d);   spool->diameter_mm     = d;
    total_txt->GetValue().ToDouble(&d);  spool->weight_total_g  = d;
    remain_txt->GetValue().ToDouble(&d); spool->weight_remain_g = d;

    if (auto den = density_txt->GetValue(); den.IsEmpty() || !den.ToDouble(&d))
        spool->density_g_cm3.reset();
    else
        spool->density_g_cm3 = d;

    long tmp;
    if (nozzle_txt->GetValue().ToLong(&tmp))
        spool->nozzle_temp_c = static_cast<int>(tmp);
    else
        spool->nozzle_temp_c = 0;
    if (nozzle_init_txt->GetValue().ToLong(&tmp))
        spool->nozzle_temp_initial_c = static_cast<int>(tmp);
    else
        spool->nozzle_temp_initial_c = 0;
    if (bed_txt->GetValue().ToLong(&tmp))
        spool->bed_temp_c = static_cast<int>(tmp);
    else
        spool->bed_temp_c = 0;
    if (bed_init_txt->GetValue().ToLong(&tmp))
        spool->bed_temp_initial_c = static_cast<int>(tmp);
    else
        spool->bed_temp_initial_c = 0;

    spool->notes    = notes_txt->GetValue().ToStdString();
    spool->archived = false;

    // Optionally apply as active print filament
    if (apply_chk->GetValue()) {
        if (auto *panel = dynamic_cast<FilamentVaultPanel *>(parent)) {
            panel->set_active_spool_id(spool->id);
            panel->apply_filament_for_printing(*spool);
        }
    }

    return true;
}

}} // namespace Slic3r::GUI
