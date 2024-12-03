import os
from ultralytics import YOLO
import torch

test_image_dir = "results/before"
test_image_name = "frisdrank-fles1.jpg"
result_image_dir = "results/after"

pretrained = False
pretrained_path = "trained.pt"
epochs = 10
save_export = True #inverted logic for some reason
mode_save_name = "trained.pt"

def main():
    if __debug__ : 
        print("Program execution started!\n")   
        print(os.getcwd())
    model = model_load()
    training_results, validation_results = train_model(model)
    save_export(model=model)  # Uncomment to enable export
    model_test(model=model, results=validation_results)

def model_load():
    if not pretrained:
        model = YOLO("yolo11n.pt")
    else:
        model = YOLO(model=pretrained_path)
    return model

# If training is enabled 
def train_model(model):
    if not pretrained:
        #training_results = model.train(data="Fles_dataset.yaml", epochs=epochs, imgsz=640)
        training_results = model.train(data="Fles_dataset.yaml", epochs=epochs, imgsz=640)
        validation_results = model.val()  # Separate validation results
        return training_results, validation_results

# If export is enabled
def save_export(model):
    if save_export:
        model.save(mode_save_name)
        #model.export(format="onnx", opset=11)

# Will test the model on image provided 
def model_test(model, results):
    result = model(test_image_dir + "/" + test_image_name)
    result[0].save(result_image_dir + "/" + test_image_name)

if __name__ == "__main__":
    main()
