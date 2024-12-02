import os
from ultralytics import YOLO

test_image_dir = "results/before"
test_image_name = "cola.jpg"
result_image_dir = "results/after"

pretrained = False
pretrained_path = ""
epochs = 20
save_export = False
mode_save_name = "trained.pt"

def main():
    if __debug__ : 
        print("Program execution started!\n");   
        print(os.getcwd())

    model = model_load()
    results = train_model(model)
    save_export(model=model)
    model_test(model=model, results= results)
      

#model load in options
def model_load():
    if not pretrained:
        model = YOLO("yolo11n.pt")
    else:
        model = YOLO(model=pretrained_path)
    return model

#if train is enabled 
def train_model(model):
    if  not pretrained:
        results = model.train(data="Fles_dataset.yaml", epochs=epochs, imgsz=640)
        results = model.val()
    return results

#if export is enabled
def save_export(model):
    if save_export:
        model.save(mode_save_name)
        model.export(format="onnx",opset=11)

#will test the model on image provided 
def model_test(model, results):
    model((test_image_dir+"/"+test_image_name))
    result = results[0]
    result.save((result_image_dir+"/"+test_image_name))


if __name__ == "__main__":
    main()