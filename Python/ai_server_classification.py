"""
나사 불량 분류 AI 서버 (이미지 전처리 버전)
일관성 향상을 위한 전처리 추가
"""

import socket
import json
import os
from ultralytics import YOLO
from PIL import Image, ImageEnhance
import io

class ScrewClassificationServer:
    def __init__(self, model_path='screw_classification/train7/weights/best.pt', port=6666):
        """AI 서버 초기화"""
        self.port = port
        
        # 모델 로드
        if not os.path.exists(model_path):
            print(f"❌ 오류: {model_path} 파일이 없습니다!")
            print("먼저 train_classification.py를 실행하여 학습하세요.")
            exit(1)
        
        print(f"✅ 모델 로드: {model_path}")
        self.model = YOLO(model_path)
        self.class_names = ['defected', 'Non_defected']
        
    def preprocess_image(self, image_path):
        """
        이미지 전처리로 일관성 향상
        - 밝기 정규화
        - 대비 조정
        - 선명도 향상
        """
        try:
            img = Image.open(image_path)
            
            # RGB 변환
            if img.mode != 'RGB':
                img = img.convert('RGB')
            
            print(f"   원본: 크기={img.size}, 모드={img.mode}")
            
            # 1. 밝기 자동 조정
            # 이미지의 평균 밝기 계산
            import numpy as np
            img_array = np.array(img)
            avg_brightness = np.mean(img_array)
            
            # 목표 밝기 (0~255 범위에서 120 정도가 적당)
            target_brightness = 120
            brightness_factor = target_brightness / avg_brightness
            brightness_factor = max(0.7, min(1.5, brightness_factor))  # 0.7~1.5 범위로 제한
            
            enhancer = ImageEnhance.Brightness(img)
            img = enhancer.enhance(brightness_factor)
            print(f"   밝기 조정: {brightness_factor:.2f}x")
            
            # 2. 대비 향상
            enhancer = ImageEnhance.Contrast(img)
            img = enhancer.enhance(1.2)
            
            # 3. 선명도 향상
            enhancer = ImageEnhance.Sharpness(img)
            img = enhancer.enhance(1.3)
            
            # 처리된 이미지 저장
            processed_path = 'processed_image.bmp'
            img.save(processed_path)
            print(f"   전처리 완료: {processed_path}")
            
            return processed_path
            
        except Exception as e:
            print(f"⚠️  전처리 실패: {e}")
            print(f"   원본 이미지 사용")
            return image_path
        
    def start(self):
        """서버 시작"""
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(('0.0.0.0', self.port))
        server.listen(5)
        
        print("=" * 50)
        print(f"🚀 나사 분류 AI 서버 시작! (전처리 버전)")
        print(f"📡 포트: {self.port}")
        print(f"🔍 분류: 불량(defected) / 정상(Non_defected)")
        print(f"🎨 전처리: 밝기/대비/선명도 자동 조정")
        print(f"⏳ 클라이언트 연결 대기 중...")
        print("=" * 50)
        
        try:
            while True:
                client, addr = server.accept()
                print(f"\n✅ 클라이언트 연결: {addr}")
                self.handle_client(client)
        except KeyboardInterrupt:
            print("\n\n서버 종료 중...")
            server.close()
    
    def handle_client(self, client):
        """클라이언트 요청 처리"""
        try:
            # 1. 이미지 크기 수신
            size_data = client.recv(4)
            if not size_data:
                print("❌ 연결 종료됨")
                return
            
            img_size = int.from_bytes(size_data, 'little')
            print(f"📥 이미지 크기: {img_size} bytes")
            
            # 2. 이미지 데이터 수신
            img_data = b''
            while len(img_data) < img_size:
                chunk = client.recv(min(8192, img_size - len(img_data)))
                if not chunk:
                    break
                img_data += chunk
            
            if len(img_data) != img_size:
                print(f"❌ 데이터 불완전: {len(img_data)}/{img_size}")
                return
            
            # 3. 원본 이미지 저장
            temp_path = 'received_image.bmp'
            with open(temp_path, 'wb') as f:
                f.write(img_data)
            print(f"💾 원본 저장: {temp_path}")
            
            # 4. 이미지 검증
            try:
                with Image.open(temp_path) as img:
                    print(f"   크기: {img.size}, 모드: {img.mode}")
            except Exception as e:
                print(f"❌ 이미지 열기 실패: {e}")
                error_response = {
                    'success': False,
                    'error': f'이미지 형식 오류: {e}'
                }
                self.send_response(client, error_response)
                return
            
            # 5. 이미지 전처리 ⭐⭐⭐
            print("🎨 이미지 전처리 중...")
            processed_path = self.preprocess_image(temp_path)
            
            # 6. AI 분류
            print("🤖 AI 분석 중...")
            results = self.model.predict(processed_path, verbose=False)
            
            # 7. 결과 추출
            probs = results[0].probs
            top1_idx = int(probs.top1)
            top1_conf = float(probs.top1conf)
            predicted_class = self.class_names[top1_idx]
            
            # 불량 여부
            is_defect = (predicted_class == 'defected')
            
            response = {
                'success': True,
                'classification': predicted_class,
                'is_defect': is_defect,
                'confidence': top1_conf,
                'result': '불량' if is_defect else '정상'
            }
            
            # 8. 신뢰도 평가
            if top1_conf < 0.70:
                confidence_level = "낮음"
                icon = "⚠️"
            elif top1_conf < 0.85:
                confidence_level = "보통"
                icon = "📊"
            else:
                confidence_level = "높음"
                icon = "✅"
            
            print(f"{icon} 분류 완료: {response['result']} ({top1_conf*100:.1f}%) [{confidence_level}]")
            
            # 9. 결과 전송
            self.send_response(client, response)
            print("📤 결과 전송 완료\n")
            
        except Exception as e:
            print(f"❌ 에러: {e}")
            import traceback
            traceback.print_exc()
            
            error_response = {
                'success': False,
                'error': str(e)
            }
            try:
                self.send_response(client, error_response)
            except:
                pass
        finally:
            client.close()
    
    def send_response(self, client, response):
        """응답 전송"""
        result_json = json.dumps(response, ensure_ascii=False).encode('utf-8')
        client.sendall(len(result_json).to_bytes(4, 'little'))
        client.sendall(result_json)

if __name__ == '__main__':
    server = ScrewClassificationServer(
        model_path='screw_classification/train7/weights/best.pt',
        port=6666
    )
    server.start()