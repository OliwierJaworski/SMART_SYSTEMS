import os
from ultralytics import YOLO

image_folder = "Data/image"
label_folder = "Data/obj_train_data" 

def main():
    print("Program execution started!\n")    
    print(os.getcwd())
    # loading Pretrained model & train it 
    model = YOLO("yolo11n.pt")
    results = model.train(data="Fles_dataset.yaml", epochs=100, imgsz=640)
    # or use your own trained one
    # model = YOLO(model="trained_models/trained_100_epochs.pt")
    # see the scores of your model after evaluation   
    results = model.val()
    # save model in the same directory as __main__.py
    model.save("fles_model.pt")
    # test model on new data
    results = model("Data/test/green_tea_pov.jpeg")
    result = results[0]
    # store the data
    result.save("output/output_image5.jpg")
    
    
if __name__ == "__main__":
    main()
