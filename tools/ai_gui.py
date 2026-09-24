import tkinter as tk
from tkinter import ttk, messagebox
import threading
import subprocess
import os

class AIGeneratorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("RawDrive AI Generator")
        self.root.geometry("450x280")
        self.root.resizable(False, False)
        
        # Style
        style = ttk.Style()
        style.theme_use('vista')
        
        # Main Frame
        frame = ttk.Frame(root, padding="15")
        frame.pack(fill=tk.BOTH, expand=True)
        
        # API Key
        ttk.Label(frame, text="API Key (OpenAI):", font=("Segoe UI", 9, "bold")).grid(row=0, column=0, sticky=tk.W, pady=(0,5))
        self.api_key_var = tk.StringVar(value=os.environ.get("OPENAI_API_KEY", ""))
        self.api_entry = ttk.Entry(frame, textvariable=self.api_key_var, show="*", width=45)
        self.api_entry.grid(row=1, column=0, columnspan=2, sticky=tk.W, pady=(0,15))
        
        # Prompt
        ttk.Label(frame, text="Prompt (Descripción del fondo):", font=("Segoe UI", 9, "bold")).grid(row=2, column=0, sticky=tk.W, pady=(0,5))
        self.prompt_var = tk.StringVar()
        self.prompt_entry = ttk.Entry(frame, textvariable=self.prompt_var, width=45)
        self.prompt_entry.grid(row=3, column=0, columnspan=2, sticky=tk.W, pady=(0,15))
        
        # Type selection
        ttk.Label(frame, text="Tipo de generación:", font=("Segoe UI", 9, "bold")).grid(row=4, column=0, sticky=tk.W, pady=(0,5))
        self.type_var = tk.StringVar(value="image")
        
        radio_frame = ttk.Frame(frame)
        radio_frame.grid(row=5, column=0, sticky=tk.W, pady=(0,15))
        ttk.Radiobutton(radio_frame, text="Imagen a Video (DALL-E 3 - Económico)", variable=self.type_var, value="image").pack(anchor=tk.W)
        ttk.Radiobutton(radio_frame, text="Video Nativo (Sora - Alta calidad)", variable=self.type_var, value="video").pack(anchor=tk.W)
        
        # Button & Status
        self.generate_btn = ttk.Button(frame, text="Generar Video", command=self.on_generate)
        self.generate_btn.grid(row=6, column=0, sticky=tk.W)
        
        self.status_var = tk.StringVar(value="Listo.")
        self.status_label = ttk.Label(frame, textvariable=self.status_var, foreground="gray")
        self.status_label.grid(row=6, column=1, sticky=tk.E)

    def on_generate(self):
        prompt = self.prompt_var.get().strip()
        if not prompt:
            messagebox.showwarning("Falta prompt", "Por favor ingresa una descripción para el fondo.")
            return
            
        api_key = self.api_key_var.get().strip()
        
        self.generate_btn.config(state=tk.DISABLED)
        self.status_var.set("Generando... (puede tardar unos minutos)")
        
        # Run in thread to not freeze UI
        threading.Thread(target=self.run_generation, args=(prompt, api_key, self.type_var.get()), daemon=True).start()

    def run_generation(self, prompt, api_key, gen_type):
        env = os.environ.copy()
        if api_key:
            env["OPENAI_API_KEY"] = api_key
            
        script_path = os.path.join(os.path.dirname(__file__), "generate_ai_media.py")
        
        try:
            # Hide console window on Windows
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            
            result = subprocess.run(
                ["python", script_path, prompt, "--type", gen_type],
                env=env,
                capture_output=True,
                text=True,
                startupinfo=startupinfo
            )
            
            if result.returncode == 0:
                self.root.after(0, self.success)
            else:
                self.root.after(0, lambda: self.fail(result.stderr or result.stdout))
        except Exception as e:
            self.root.after(0, lambda: self.fail(str(e)))

    def success(self):
        self.status_var.set("¡Completado!")
        self.generate_btn.config(state=tk.NORMAL)
        messagebox.showinfo("Éxito", "Video generado correctamente.\n\nAbre el menú del System Tray y ve a 'Select Video' para aplicarlo al instante.")
        
    def fail(self, error_msg):
        self.status_var.set("Error.")
        self.generate_btn.config(state=tk.NORMAL)
        messagebox.showerror("Error de generación", f"Ocurrió un error:\n\n{error_msg}")

if __name__ == "__main__":
    root = tk.Tk()
    app = AIGeneratorGUI(root)
    root.mainloop()
