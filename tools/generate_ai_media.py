import argparse
import os
import shutil
import time
import urllib.request
import json
from pathlib import Path

# Configuración
OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY", "")
OUTPUT_DIR = Path("videos")
ASSETS_DIR = Path("assets")

def ensure_dir(path):
    if not path.exists():
        path.mkdir(parents=True)

def mock_generation(prompt, output_file, mode):
    print(f"[*] (MOCK MODE) No se detectó OPENAI_API_KEY.")
    print(f"[*] Simulando la generación de {mode} para el prompt: '{prompt}'")
    time.sleep(2)
    
    # Copiamos el video por defecto como si fuera el resultado generado
    default_video = OUTPUT_DIR / "garage_loop.mp4"
    if default_video.exists():
        shutil.copy(default_video, output_file)
        print(f"[+] Video simulado guardado en: {output_file}")
    else:
        print("[-] Error: No se encontró garage_loop.mp4 para el mock.")
        return False
    return True

def generate_openai_image_and_convert(prompt, output_file):
    print("[*] Solicitando imagen a DALL-E 3...")
    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {OPENAI_API_KEY}"
    }
    payload = {
        "model": "dall-e-3",
        "prompt": prompt,
        "n": 1,
        "size": "1024x1024"
    }
    
    response = requests.post("https://api.openai.com/v1/images/generations", headers=headers, json=payload)
    if response.status_code != 200:
        print(f"[-] Error de API: {response.text}")
        return False
        
    data = response.json()
    img_url = data['data'][0]['url']
    print("[+] Imagen generada. Descargando...")
    
    temp_img = "temp_dalle_img.png"
    urllib.request.urlretrieve(img_url, temp_img)
    
    print("[*] Convirtiendo imagen a MP4 en bucle...")
    # Requiere imageio e imageio-ffmpeg
    try:
        import imageio.v3 as iio
        import numpy as np
    except ImportError:
        print("[-] Faltan dependencias. Instala: pip install imageio[ffmpeg] numpy pillow")
        return False
        
    try:
        img = iio.imread(temp_img)
        # Crear un video de 3 segundos a 30fps
        fps = 30
        duration = 3
        total_frames = fps * duration
        
        # Guardar como h264 mp4
        writer = iio.imopen(output_file, "w", plugin="pyav")
        writer.init_video_stream("h264", fps=fps)
        
        for i in range(total_frames):
            writer.write_frame(img)
            
        writer.close()
        print(f"[+] Video MP4 generado exitosamente: {output_file}")
    except Exception as e:
        print(f"[-] Error durante la conversión: {e}")
        return False
    finally:
        if os.path.exists(temp_img):
            os.remove(temp_img)
            
    return True

def generate_openai_sora(prompt, output_file):
    # NOTA: Sora aún no tiene un endpoint público general, esto es una estructura representativa.
    print("[*] Solicitando video a OpenAI Sora...")
    print("[-] El endpoint de Sora no está disponible públicamente aún. Usando mock...")
    return mock_generation(prompt, output_file, "video")

def main():
    parser = argparse.ArgumentParser(description="Generador de Media IA para RawDrive Engine")
    parser.add_argument("prompt", type=str, help="El prompt de texto para la IA")
    parser.add_argument("--type", choices=["image", "video"], default="image", 
                        help="Tipo de generación: 'image' (DALL-E 3 -> MP4) o 'video' (Sora)")
    
    args = parser.parse_args()
    
    ensure_dir(OUTPUT_DIR)
    
    timestamp = int(time.time())
    output_filename = f"ai_gen_{args.type}_{timestamp}.mp4"
    output_path = OUTPUT_DIR / output_filename
    
    print(f"--- RawDrive AI Generator ---")
    print(f"Prompt: {args.prompt}")
    print(f"Tipo: {args.type}")
    print(f"-----------------------------\n")
    
    if not OPENAI_API_KEY:
        success = mock_generation(args.prompt, output_path, args.type)
    else:
        # Importamos requests aquí por si corre en MOCK MODE sin dependencias instaladas
        global requests
        import requests
        
        if args.type == "image":
            success = generate_openai_image_and_convert(args.prompt, output_path)
        else:
            success = generate_openai_sora(args.prompt, output_path)
            
    if success:
        print(f"\n[OK] El video está listo. Abre el System Tray de RawDrive y selecciona '{output_filename}' en 'Select Video'.")
    else:
        print("\n[ERROR] Ocurrió un problema durante la generación.")

if __name__ == "__main__":
    main()
