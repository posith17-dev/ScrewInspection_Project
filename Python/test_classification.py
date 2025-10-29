"""
학습된 나사 분류 모델 테스트
"""

from ultralytics import YOLO
import os

def test_classification():
    """
    나사 불량/정상 분류 테스트
    """
    print("=" * 50)
    print("나사 불량 분류 테스트")
    print("=" * 50)
    
    # 1. 학습된 모델 로드
    model_path = 'screw_classification/train7/weights/best.pt'
    
    if not os.path.exists(model_path):
        print(f"❌ 오류: {model_path} 파일이 없습니다!")
        print("먼저 train_classification.py를 실행하여 학습하세요.")
        return
    
    print(f"\n✅ 모델 로드: {model_path}")
    model = YOLO(model_path)
    
    # 2. 테스트 이미지 찾기
    test_dirs = [
        'Screw.v3i.folder/test/defected',
        'Screw.v3i.folder/test/Non_defected'
    ]
    
    test_images = []
    for test_dir in test_dirs:
        if os.path.exists(test_dir):
            files = [os.path.join(test_dir, f) for f in os.listdir(test_dir) 
                    if f.endswith(('.jpg', '.png', '.jpeg'))]
            test_images.extend(files[:5])  # 각 폴더에서 5개씩
    
    if not test_images:
        print("❌ 테스트 이미지를 찾을 수 없습니다!")
        return
    
    print(f"✅ 테스트 이미지: {len(test_images)}개")
    
    # 3. 추론
    print("\n" + "=" * 50)
    print("테스트 결과:")
    print("=" * 50)
    
    for img_path in test_images:
        results = model.predict(img_path, verbose=False)
        
        # 결과 추출
        probs = results[0].probs
        top1_idx = int(probs.top1)
        top1_conf = float(probs.top1conf)
        
        # 클래스 이름
        class_names = ['defected', 'Non_defected']
        predicted_class = class_names[top1_idx]
        
        # 실제 클래스 (폴더명에서)
        actual_class = 'defected' if 'defected' in img_path else 'Non_defected'
        
        # 정답 여부
        is_correct = "✅" if predicted_class == actual_class else "❌"
        
        print(f"\n이미지: {os.path.basename(img_path)}")
        print(f"  실제: {actual_class}")
        print(f"  예측: {predicted_class} ({top1_conf*100:.1f}%) {is_correct}")
    
    print("\n" + "=" * 50)
    print("테스트 완료!")
    print("=" * 50)

if __name__ == '__main__':
    test_classification()