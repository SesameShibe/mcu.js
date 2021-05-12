os.initFont()
manager = new UIManager()

root = new View()
root.setSize(240, 240)
manager.views.push(root)

lbl = new Label()
lbl.setSize(100, 100)
lbl.setLocation(1, 1)
root.addView(lbl)
manager.draw()