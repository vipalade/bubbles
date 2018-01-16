//
//  DrawView.swift
//  bubbles
//
//  Created by Valentin Palade on 06/01/2018.
//  Copyright © 2018 Valentin Palade. All rights reserved.
//

import UIKit
import os.log

class DrawView: UIView {
    var position: CGPoint = CGPoint(x: 0, y: 0)
    let local_diameter: CGFloat = 60.0
    let remote_diameter: CGFloat = 30.0
    
    var has_touch = false
    
    override init(frame: CGRect) {
        super.init(frame: frame)
    }
    
    required init?(coder: NSCoder){
        super.init(coder: coder)
        position = super.center
    }
    
    public func onLoad() {
        position = super.center
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        has_touch = true
        position = touches.first!.location(in: self)
        //os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        let x = engine_reverse_scale_x(((position.x - self.bounds.width/2) as NSNumber).int32Value, Int32(self.bounds.width))
        let y = engine_reverse_scale_y(((position.y - self.bounds.height/2) as NSNumber).int32Value, Int32(self.bounds.height))
        engine_move(x, y)
        position.x = CGFloat(x)
        position.y = CGFloat(y)
        self.setNeedsDisplay()
    }
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        has_touch = false
        position = touches.first!.location(in: self)
        //os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        let x = engine_reverse_scale_x(((position.x - self.bounds.width/2) as NSNumber).int32Value, Int32(self.bounds.width))
        let y = engine_reverse_scale_y(((position.y - self.bounds.height/2) as NSNumber).int32Value, Int32(self.bounds.height))
        engine_move(x, y)
        position.x = CGFloat(x)
        position.y = CGFloat(y)
        self.setNeedsDisplay()
    }
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        position = touches.first!.location(in: self)
        //os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        let x = engine_reverse_scale_x(((position.x - self.bounds.width/2) as NSNumber).int32Value, Int32(self.bounds.width))
        let y = engine_reverse_scale_y(((position.y - self.bounds.height/2) as NSNumber).int32Value, Int32(self.bounds.height))
        engine_move(x, y)
        position.x = CGFloat(x)
        position.y = CGFloat(y)
        self.setNeedsDisplay()
    }
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        //nothing here yet
    }
    func uiColorFromHex(rgbValue: Int32) -> UIColor {
        // & binary AND operator to zero out other color values
        // >> bitwise right shift operator
        // Divide by 0xFF because UIColor takes CGFloats between 0.0 and 1.0
        let red = CGFloat((rgbValue & 0xFF0000) >> 16) / 0xFF
        let green = CGFloat((rgbValue & 0x00FF00) >> 8) / 0xFF
        let blue = CGFloat(rgbValue & 0x0000FF) / 0xFF
        let alpha = CGFloat(1.0)
        return UIColor(red: red, green: green, blue: blue, alpha: alpha)
    }
    
    func updateGUI(){
        DispatchQueue.main.async(execute: {
            self.setNeedsDisplay()//needs to be called from the main thread
        })
    }
    
    func drawCircle(x: Int32, y: Int32, color: Int32, diameter: CGFloat){
        //os_log("drawCircle: x = %@; y = %@; c = %@; d = %@", log: OSLog.default, type: .debug, x.description, y.description, color.description, diameter.description)
        let nx = engine_scale_x(x, Int32(floor(self.bounds.width)))
        let ny = engine_scale_y(y, Int32(floor(self.bounds.height)))
        let path = UIBezierPath(ovalIn: CGRect(x: CGFloat(nx) + self.center.x - diameter/2, y: CGFloat(ny) + self.center.y - diameter/2, width: diameter, height: diameter))
        let ncolor = uiColorFromHex(rgbValue: color)
        ncolor.setStroke()
        ncolor.setFill()
        path.lineWidth = 5
        path.stroke()
        path.fill()
    }
    
    
    // Only override draw() if you perform custom drawing.
    // An empty implementation adversely affects performance during animation.
    override func draw(_ rect: CGRect) {
        //os_log("draw", log: OSLog.default, type: .debug)
        engine_plot_start()
        
        let my_color = engine_plot_my_color()
        
        while engine_plot_end() != 1{
            let x = engine_plot_x()
            let y = engine_plot_y()
            let c = engine_plot_color()
            
            drawCircle(x:x, y:y, color:c, diameter:remote_diameter)
            engine_plot_next()
        }
        
        drawCircle(x: (position.x as NSNumber).int32Value, y:(position.y as NSNumber).int32Value, color:my_color, diameter:local_diameter)
        engine_plot_done()
    }
    
    func autoMove(x: Int32, y: Int32){
        if !has_touch{
            //os_log("autoMove: x = %@; y = %@ has_touch = %@", log: OSLog.default, type: .debug, x.description, y.description, has_touch.description)
            position.x = CGFloat(x)
            position.y = CGFloat(y)
            engine_move(x, y)
            
            DispatchQueue.main.async(execute: {
                self.setNeedsDisplay()//needs to be called from the main thread
            })
        }
    }
}
