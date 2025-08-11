import sys
from pathlib import Path
import torch
import cv2
import argparse
import os

# YOLOv5 라이브러리 경로 설정
FILE = Path(__file__).resolve()
ROOT = FILE.parents[0]
if str(ROOT) not in sys.path:
    sys.path.append(str(ROOT))

from models.common import DetectMultiBackend
from utils.dataloaders import LoadStreams
from utils.general import (LOGGER, check_img_size, non_max_suppression, scale_boxes, select_device, smart_inference_mode)
from utils.plots import Annotator, colors

def get_valid_bus_number():
    """사용자로부터 4자리 버스 번호를 입력받아 반환합니다."""
    while True:
        try:
            bus_num = int(input("탑승할 버스 번호를 입력하세요 (4자리): "))
            if 0 <= bus_num <= 9999:
                return str(bus_num) # 파일명으로 사용하기 위해 문자열로 반환
            print("오류: 0~9999 사이의 유효한 숫자를 입력해주세요.")
        except ValueError:
            print("오류: 숫자를 입력해주세요.")

@smart_inference_mode()
def run(weights='wheel.pt', source='0', imgsz=(640, 480), conf_thres=0.25, iou_thres=0.45, device='', view_img=True):
    """
    YOLO 모델을 사용하여 웹캠에서 객체를 감지하고, 사용자 입력에 따라 이미지를 저장하는 핵심 함수입니다.
    """
    # 1. 모델과 디바이스 설정: 'wheel.pt' 모델을 로드하고 GPU 또는 CPU를 선택합니다.
    device = select_device(device)
    model = DetectMultiBackend(weights, device=device)
    stride, names, pt = model.stride, model.names, model.pt
    imgsz = check_img_size(imgsz, s=stride)

    # 2. 웹캠 데이터 로더 설정: 0번 웹캠에서 영상을 실시간으로 가져옵니다.
    dataset = LoadStreams(source, img_size=imgsz, stride=stride, auto=pt)
    
    # 3. 사용자로부터 버스 번호 입력: 프로그램 시작 시 단 한 번만 입력받습니다.
    bus_number = get_valid_bus_number()
    LOGGER.info(f"선택된 버스 번호: {bus_number}")

    # 4. 이미지 저장 경로 설정: 버스 번호에 해당하는 폴더를 만들고, 저장할 파일 경로를 지정합니다.
    captures_dir = Path(f'/home/bustacall/yolov5/{bus_number}')
    captures_dir.mkdir(parents=True, exist_ok=True)
    capture_path = captures_dir / f'{bus_number}.jpg'
    
    image_captured = False # 이미지 캡처 상태를 추적하는 플래그

    # 5. 메인 추론 루프: 웹캠에서 프레임을 하나씩 읽어와 처리합니다.
    for path, im, im0s, _, s in dataset:
        im = torch.from_numpy(im).to(device)
        im = im.float() / 255.0
        if len(im.shape) == 3: im = im[None]

        pred = model(im)
        pred = non_max_suppression(pred, conf_thres, iou_thres)

        # 감지 결과 처리: 감지된 객체에 바운딩 박스를 그립니다.
        annotator = Annotator(im0s[0], example=str(names))
        if len(pred[0]):
            det = pred[0]
            det[:, :4] = scale_boxes(im.shape[2:], det[:, :4], im0s[0].shape).round()
            for *xyxy, conf, cls in det:
                label = f'{names[int(cls)]} {conf:.2f}'
                annotator.box_label(xyxy, label, color=colors(int(cls), True))
        
        im0 = annotator.result()

        # 6. 이미지 캡처: 객체 감지가 이루어진 최초 한 번만 이미지를 저장합니다.
        if not image_captured and len(pred[0]) > 0:
            cv2.imwrite(str(capture_path), im0)
            LOGGER.info(f'객체 감지 후 이미지를 저장했습니다: {capture_path}')
            image_captured = True
            
        # 7. 화면에 결과 표시: 실시간으로 처리된 영상을 화면에 보여줍니다.
        if view_img:
            cv2.imshow("Bus Support System", im0)
            if cv2.waitKey(1) == ord('q'): # 'q' 키를 누르면 종료
                break
    
    cv2.destroyAllWindows()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--weights', type=str, default=ROOT / 'wheel.pt')
    parser.add_argument('--source', type=str, default='0')
    opt = parser.parse_args()
    
    run(**vars(opt))
