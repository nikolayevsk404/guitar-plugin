import react from '@vitejs/plugin-react'
import { defineConfig } from 'vite'
import { viteSingleFile } from 'vite-plugin-singlefile'

// Bundla tudo (JS/CSS) em um unico index.html -- o PluginEditor (C++) embute esse arquivo
// como BinaryData e o serve via WebBrowserComponent::Resource, sem depender de arquivos
// externos soltos ao lado do binario do plugin (ver src/plugin/PluginEditor.cpp).
export default defineConfig({
  plugins: [react(), viteSingleFile()],
})
