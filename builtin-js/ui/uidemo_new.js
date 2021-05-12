mngr = new ScreenManager()

root = new View()
root.setSize(240, 240)
mngr.views.push(root)

lbl = new Label()
lbl.setSize(100, 100)
root.addView(lbl)