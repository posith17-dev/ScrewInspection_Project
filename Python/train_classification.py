"""
나사 불량 분류 모델 학습
YOLOv8 Classification
"""

from ultralytics import YOLO

def train_screw_classification():
    """
    나사 불량/정상 분류 모델 학습
    """
    print("=" * 50)
    print("나사 불량 분류 모델 학습 시작")
    print("=" * 50)
    
    # 1. 분류 모델 로드
    print("\n[1/3] YOLOv8 분류 모델 로드 중...")
    model = YOLO('yolov8s-cls.pt') # small 모델 사용
    print("✅ 모델 로드 완료")
    
    # 2. 학습 시작
    print("\n[2/3] 학습 시작...")
    print("⏳ CPU 모드: 약 2-4시간 소요 예상")
    print("💡 Ctrl+C로 언제든지 중단 가능")
    
    results = model.train(
        data='Screw.v3i.folder',  # 폴더 경로만 지정
        epochs=300,                 # 에포크 수
        imgsz=224,                 # 분류용 이미지 크기
        batch=8,                   # 배치 크기
        device='cpu',              # CPU 사용
        patience=0,                 # 조기 종료 비활성화(=0)
        save=True,
        plots=True,
        verbose=True,
        project='screw_classification',
        name='train'
    )
    
    print("\n[3/3] 학습 완료!")
    print(f"✅ 최고 성능 모델: runs/classify/train/weights/best.pt")
    print(f"✅ 마지막 모델: runs/classify/train/weights/last.pt")
    
    # 검증
    print("\n모델 검증 중...")
    metrics = model.val()
    
    print("\n" + "=" * 50)
    print("학습 결과:")
    print("=" * 50)
    print(f"정확도(Accuracy): {metrics.top1:.3f}")
    print(f"Top5 정확도: {metrics.top5:.3f}")
    print("=" * 50)
    
    print("\n✅ 모든 작업 완료!")
    print("\n클래스:")
    print("  0: defected (불량)")
    print("  1: Non_defected (정상)")

if __name__ == '__main__':
    try:
        train_screw_classification()
    except KeyboardInterrupt:
        print("\n\n⚠️  학습이 중단되었습니다.")
        print("중간 저장된 모델:")
        print("  runs/classify/train/weights/last.pt")
