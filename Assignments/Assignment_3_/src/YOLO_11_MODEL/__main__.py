import os
from ultralytics import YOLO

image_folder = "Data/image"
label_folder = "Data/obj_train_data" 

def main():
    print("Program execution started!\n")    
    print(os.getcwd())
    # loading Pretrained model
    #model = YOLO("yolo11n.pt")
    model = YOLO(model="trained_models/trained_100_epochs.pt")
    #results = model.train(data="Fles_dataset.yaml", epochs=100, imgsz=640)
    #results = model.val()
    #model.save("fles_model.pt")
    results = model("Data/test/green_tea_pov.jpeg")
    result = results[0]
    result.save("output/output_image5.jpg")
if __name__ == "__main__":
    main()
