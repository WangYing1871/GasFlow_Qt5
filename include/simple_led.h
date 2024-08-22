#ifndef simple_led_H
#define simple_led_H 1 
#include <type_traits>

#include "QAbstractButton"
#include "QTimer"
class simple_led : public QAbstractButton{
  Q_OBJECT

  typedef struct{
    QColor m_on0, m_on1;
    QColor m_off0, m_off1;
  } color_group_t;

public:
  enum class e_color : uint8_t{
    k_custom = 0
    ,k_red
    ,k_green
    ,k_blue
    ,k_yellow
    ,k_orange
  };
  enum class e_status : uint8_t{
    k_on = 0
    ,k_off
    ,k_blink
  };

public:
  typedef simple_led self_t;
  typedef QAbstractButton base_t;

  simple_led(QWidget* parent=nullptr, e_color c = e_color::k_green);
  e_status states() const {return m_status;}
  void status(e_status);
  void interval(size_t msec) {m_interval = msec;}

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void resizeEvent(QResizeEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

private slots:
  void blink_timer_timeout();

private:
  void reset_status();

  e_status m_status = e_status::k_off;
  e_color m_color = e_color::k_green;
  QTimer* m_blink_timer = nullptr;
  size_t m_interval = 200;

private:
  static color_group_t sm_color_palette[];
public:
  static void set_custom_on_color0(QColor const& c) {
    sm_color_palette[(std::underlying_type<e_color>::type)(e_color::k_custom)].m_on0 = c;}
  static void set_custom_on_color1(QColor const& c) {
    sm_color_palette[(std::underlying_type<e_color>::type)(e_color::k_custom)].m_on1 = c;}
  static void set_custom_off_color0(QColor const& c) {
    sm_color_palette[(std::underlying_type<e_color>::type)(e_color::k_custom)].m_off0 = c;}
  static void set_custom_off_color1(QColor const& c) {
    sm_color_palette[(std::underlying_type<e_color>::type)(e_color::k_custom)].m_off1 = c;}
};
#endif
