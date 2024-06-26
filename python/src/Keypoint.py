import Core

class Keypoint():
  def __init__(self, y=0, x=0, resp=0):
    self.y = y
    self.x = x
    self.resp = resp

def PlotKp(img, kp_list):
  img_aux = img.copy()
  for kp in kp_list:
    Core.cv2.circle(img=img_aux, center=(kp.x, kp.y), radius=4, color=(255, 255, 255))

  return img_aux
  