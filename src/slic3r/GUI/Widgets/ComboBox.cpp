#include "ComboBox.hpp"
#include "UIColors.hpp"

#include <wx/dcgraph.h>
#include <wx/choice.h>
#if wxUSE_ACCESSIBILITY
#include <wx/access.h>
#endif

#include "../GUI_App.hpp"

#if wxUSE_ACCESSIBILITY
// Supplies a fixed accessible name; role, value and items come from the native control.
class NativeComboAccessible : public wxAccessible
{
    wxString m_name;

public:
    NativeComboAccessible(wxWindow *win, const wxString &name) : wxAccessible(win), m_name(name) {}

    wxAccStatus GetName(int childId, wxString *name) override
    {
        if (childId != wxACC_SELF)
            return wxACC_NOT_IMPLEMENTED;
        *name = m_name;
        return wxACC_OK;
    }
};
#endif

BEGIN_EVENT_TABLE(ComboBox, TextInput)

EVT_LEFT_DOWN(ComboBox::mouseDown)
EVT_MOUSEWHEEL(ComboBox::mouseWheelMoved)
EVT_KEY_DOWN(ComboBox::keyDown)

// catch paint events
END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

ComboBox::ComboBox(wxWindow *      parent,
                   wxWindowID      id,
                   const wxString &value,
                   const wxPoint & pos,
                   const wxSize &  size,
                   int             n,
                   const wxString  choices[],
                   long            style)
    : drop(texts, icons)
{
    text_off = style & CB_NO_TEXT;
    TextInput::Create(parent, "", value, (style & CB_NO_DROP_ICON) ? "" : "drop_down", pos, size,
                      style | wxTE_PROCESS_ENTER);
    drop.Create(this, style);

    SetFont(Slic3r::GUI::wxGetApp().normal_font());
    if (style & wxCB_READONLY)
        GetTextCtrl()->Hide();
    else
        GetTextCtrl()->Bind(wxEVT_KEY_DOWN, &ComboBox::keyDown, this);

    SetBorderColor(TextInput::GetBorderColor());
    if (parent) {
        SetBackgroundColour(parent->GetBackgroundColour());
        SetForegroundColour(parent->GetForegroundColour());
    }

    drop.Bind(wxEVT_COMBOBOX, [this](wxCommandEvent &e) {
        SetSelection(e.GetInt());
        e.SetEventObject(this);
        e.SetId(GetId());
        GetEventHandler()->ProcessEvent(e);
    });
    drop.Bind(EVT_DISMISS, [this](auto &) {
        drop_down = false;
        wxCommandEvent e(wxEVT_COMBOBOX_CLOSEUP);
        GetEventHandler()->ProcessEvent(e);
    });

#ifndef _WIN32
    this->Bind(wxEVT_SYS_COLOUR_CHANGED, [this, parent](wxSysColourChangedEvent& event) {
        event.Skip();
        SetBackgroundColour(parent->GetBackgroundColour());
        SetForegroundColour(parent->GetForegroundColour());
    });
#endif
    for (int i = 0; i < n; ++i) Append(choices[i]);
}

void ComboBox::UseNativeControl(const wxString &accessible_name)
{
    if (native)
        return;
    native = new wxChoice(this, wxID_ANY, wxDefaultPosition, GetClientSize());
    for (const wxString &text : texts)
        native->Append(text);
    native->SetSelection(drop.GetSelection());
#if wxUSE_ACCESSIBILITY
    native->SetAccessible(new NativeComboAccessible(native, accessible_name));
#endif
    SetMinSize(wxSize(GetMinSize().x, std::max(GetMinSize().y, native->GetBestSize().y)));

    native->Bind(wxEVT_CHOICE, [this](wxCommandEvent &e) {
        drop.SetSelection(e.GetInt());
        TextInput::SetLabel(drop.GetValue());
        sendComboBoxEvent();
    });
    Bind(wxEVT_SIZE, [this](wxSizeEvent &e) {
        native->SetSize(GetClientSize());
        e.Skip();
    });
}

int ComboBox::GetSelection() const
{
    return drop.GetSelection();
}

void ComboBox::SetSelection(int n)
{
    drop.SetSelection(n);
    SetLabel(drop.GetValue());
    if (drop.selection >= 0)
        SetIcon(icons[drop.selection]);
    if (native)
        native->SetSelection(drop.GetSelection());
}

void ComboBox::Rescale()
{
    SetFont(Slic3r::GUI::wxGetApp().normal_font());

    TextInput::Rescale();
    drop.Rescale();
}

wxString ComboBox::GetValue() const
{
    return drop.GetSelection() >= 0 ? drop.GetValue() : GetLabel();
}

void ComboBox::SetValue(const wxString &value)
{
    drop.SetValue(value);
    SetLabel(value);
    if (drop.selection >= 0)
        SetIcon(icons[drop.selection]);
    if (native)
        native->SetSelection(drop.GetSelection());
}

void ComboBox::SetLabel(const wxString &value)
{
    if (GetTextCtrl()->IsShown() || text_off)
        GetTextCtrl()->SetValue(value);
    else
        TextInput::SetLabel(value);
}

wxString ComboBox::GetLabel() const
{
    if (GetTextCtrl()->IsShown() || text_off)
        return GetTextCtrl()->GetValue();
    else
        return TextInput::GetLabel();
}

void ComboBox::SetTextLabel(const wxString& label)
{
    TextInput::SetLabel(label);
}

wxString ComboBox::GetTextLabel() const
{
    return TextInput::GetLabel();
}

bool ComboBox::SetFont(wxFont const& font)
{
    const bool set_drop_font = drop.SetFont(font);
    if (GetTextCtrl() && GetTextCtrl()->IsShown())
        return GetTextCtrl()->SetFont(font) && set_drop_font;
    return TextInput::SetFont(font) && set_drop_font;
}

bool ComboBox::SetBackgroundColour(const wxColour& colour)
{
    TextInput::SetBackgroundColour(colour);

    drop.SetBackgroundColour(colour);
    StateColor selector_colors( std::make_pair(Slic3r::GUI::Widget::get_clr_background_focused(),          (int)StateColor::Checked),
        Slic3r::GUI::wxGetApp().dark_mode() ?
                                std::make_pair(Slic3r::GUI::Widget::clr_background_disabled_dark,    (int)StateColor::Disabled) :
                                std::make_pair(Slic3r::GUI::Widget::clr_background_disabled_light,   (int)StateColor::Disabled),
        Slic3r::GUI::wxGetApp().dark_mode() ?
                                std::make_pair(Slic3r::GUI::Widget::clr_background_normal_dark,      (int)StateColor::Normal) :
                                std::make_pair(Slic3r::GUI::Widget::clr_background_normal_light,     (int)StateColor::Normal));
    drop.SetSelectorBackgroundColor(selector_colors);

    return true;
}

bool ComboBox::SetForegroundColour(const wxColour& colour)
{
    TextInput::SetForegroundColour(colour);

    drop.SetTextColor(TextInput::GetTextColor());

    return true;
}

void ComboBox::SetBorderColor(StateColor const& color)
{
    TextInput::SetBorderColor(color);
    drop.SetBorderColor(color);
    drop.SetSelectorBorderColor(color);
}

int ComboBox::Append(const wxString &item, const wxBitmapBundle &bitmap)
{
    return Append(item, bitmap, nullptr);
}

int ComboBox::Append(const wxString         &item,
                     const wxBitmapBundle   &bitmap,
                     void *                 clientData)
{
    texts.push_back(item);
    icons.push_back(bitmap);
    datas.push_back(clientData);
    types.push_back(wxClientData_None);
    drop.Invalidate();
    if (native)
        native->Append(item);
    return int(texts.size()) - 1;
}

int ComboBox::Insert(const wxString& item, 
                     const wxBitmapBundle& bitmap,
                     unsigned int pos)
{
    return Insert(item, bitmap, pos, nullptr);
}

int ComboBox::Insert(const wxString& item, const wxBitmapBundle& bitmap,
    unsigned int pos, void* clientData)
{
    const int n = wxItemContainer::Insert(item, pos, clientData);
    if (n != wxNOT_FOUND)
        icons[n] = bitmap;
    return n;
}

void ComboBox::DoClear()
{
    texts.clear();
    icons.clear();
    datas.clear();
    types.clear();
    drop.Invalidate(true);
    if (GetTextCtrl()->IsShown() || text_off)
        GetTextCtrl()->Clear();
    if (native)
        native->Clear();
}

void ComboBox::DoDeleteOneItem(unsigned int pos)
{
    if (pos >= texts.size()) return;
    texts.erase(texts.begin() + pos);
    icons.erase(icons.begin() + pos);
    datas.erase(datas.begin() + pos);
    types.erase(types.begin() + pos);
    const int selection = drop.GetSelection();
    drop.Invalidate(true);
    drop.SetSelection(selection);
    if (native) {
        native->Delete(pos);
        native->SetSelection(drop.GetSelection());
    }
}

unsigned int ComboBox::GetCount() const { return texts.size(); }

wxString ComboBox::GetString(unsigned int n) const
{
    return n < texts.size() ? texts[n] : wxString{};
}

void ComboBox::SetString(unsigned int n, wxString const &value)
{
    if (n >= texts.size()) return;
    texts[n]  = value;
    drop.Invalidate();
    if (int(n) == drop.GetSelection()) SetLabel(value);
    if (native)
        native->SetString(n, value);
}

wxBitmap ComboBox::GetItemBitmap(unsigned int n) 
{
    return icons[n].GetBitmapFor(m_parent);
}

void ComboBox::OnKeyDown(wxKeyEvent &event)
{
    keyDown(event);
}

int ComboBox::DoInsertItems(const wxArrayStringsAdapter &items,
                            unsigned int                 pos,
                            void **                      clientData,
                            wxClientDataType             type)
{
    if (pos > texts.size()) return -1;
    for (size_t i = 0; i < items.GetCount(); ++i) {
        texts.insert(texts.begin() + pos, items[i]);
        icons.insert(icons.begin() + pos, wxNullBitmap);
        datas.insert(datas.begin() + pos, clientData ? clientData[i] : NULL);
        types.insert(types.begin() + pos, type);
        if (native)
            native->Insert(items[i], pos);
        ++pos;
    }
    const int selection = drop.GetSelection();
    drop.Invalidate(true);
    drop.SetSelection(selection);
    if (native)
        native->SetSelection(drop.GetSelection());
    return int(pos) - 1;
}

void *ComboBox::DoGetItemClientData(unsigned int n) const { return n < texts.size() ? datas[n] : NULL; }

void ComboBox::DoSetItemClientData(unsigned int n, void *data)
{
    if (n < texts.size())
        datas[n] = data;
}

void ComboBox::mouseDown(wxMouseEvent &event)
{
    SetFocus();
    if (drop_down) {
        drop.Hide();
    } else if (drop.HasDismissLongTime()) {
        drop.autoPosition();
        drop_down = true;
        drop.Popup();
        wxCommandEvent e(wxEVT_COMBOBOX_DROPDOWN);
        GetEventHandler()->ProcessEvent(e);
    }
}

void ComboBox::mouseWheelMoved(wxMouseEvent &event)
{
    if (drop_down) return;
    auto delta = ((event.GetWheelRotation() < 0) == event.IsWheelInverted()) ? -1 : 1;
    unsigned int n = GetSelection() + delta;
    if (n < GetCount()) {
        SetSelection((int) n);
        sendComboBoxEvent();
    }
}

void ComboBox::keyDown(wxKeyEvent& event)
{
    int key_code = event.GetKeyCode();
    switch (key_code) {
        case WXK_RETURN: {
            if (drop_down) {
                drop.DismissAndNotify();
                sendComboBoxEvent();
            }
            else if (drop.HasDismissLongTime()) {
                drop.autoPosition();
                drop_down = true;
                drop.Popup();
                wxCommandEvent e(wxEVT_COMBOBOX_DROPDOWN);
                GetEventHandler()->ProcessEvent(e);
            }
            break;
        }
        case WXK_UP: {
            if (GetSelection() > 0)
                SetSelection(GetSelection() - 1);
            if (!drop.IsShown())
                sendComboBoxEvent();
            break;
        }
        case WXK_DOWN: {
            if (GetSelection() + 1 < int(texts.size()))
                SetSelection(GetSelection() + 1);
            if (!drop.IsShown())
                sendComboBoxEvent();
            break;
        }
        case WXK_LEFT: {
            if (HasFlag(wxCB_READONLY)) {
                if(GetSelection() > 0)
                    SetSelection(GetSelection() - 1);
                break;
            }
            const auto pos = GetTextCtrl()->GetInsertionPoint();
            if(pos > 0)
                GetTextCtrl()->SetInsertionPoint(pos - 1);
            break;
        }
        case WXK_RIGHT: {
            if (HasFlag(wxCB_READONLY)) {
                if (GetSelection() + 1 < int(texts.size()))
                    SetSelection(GetSelection() + 1);
                break;
            }
            const size_t pos = size_t(GetTextCtrl()->GetInsertionPoint());
            if (pos < GetLabel().Length())
                GetTextCtrl()->SetInsertionPoint(pos + 1);
            break;
        }
        case WXK_TAB:
            HandleAsNavigationKey(event);
            break;
        default: {
            if (drop.IsShown() && HasFlag(wxCB_READONLY)) {
                for (size_t n = 0; n < texts.size(); n++) {
                    if (texts[n].StartsWith(wxString(static_cast<char>(key_code)))) {
                        SetSelection(int(n));
                        break;
                    }
                }
            }
            event.Skip();
            break;
        }
    }
}

void ComboBox::OnEdit()
{
    auto value = GetTextCtrl()->GetValue();
    SetValue(value);
}

#ifdef __WIN32__

WXLRESULT ComboBox::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if (nMsg == WM_GETDLGCODE) {
        return DLGC_WANTALLKEYS;
    }
    return TextInput::MSWWindowProc(nMsg, wParam, lParam);
}

#endif

void ComboBox::sendComboBoxEvent()
{
    wxCommandEvent event(wxEVT_COMBOBOX, GetId());
    event.SetEventObject(this);
    event.SetInt(drop.GetSelection());
    event.SetString(drop.GetValue());
    GetEventHandler()->ProcessEvent(event);
}
